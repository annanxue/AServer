module( "db_activity", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert

------------------------------GROUP BUY------------------------------

SELECT_SERVER_GROUP_BUY_SQL = [[
    SELECT
    b.group_buy as group_buy
    from PLAYER_TBL a, GROUP_BUY_TBL b where a.player_id = b.player_id and unix_timestamp(a.last_update_time) > ?;
]]
SELECT_SERVER_GROUP_BUY_SQL_PARAM_TYPE = "i"
SELECT_SERVER_GROUP_BUY_SQL_RESULT_TYPE = "s"


SELECT_GROUP_BUY_SQL = [[
    SELECT
    `group_buy`
    from GROUP_BUY_TBL where player_id = ?;
]]

SELECT_GROUP_BUY_SQL_RESULT_TYPE = "s"

REPLACE_GROUP_BUY_SQL = [[
    REPLACE INTO GROUP_BUY_TBL ( `player_id`, `group_buy` ) VALUES
    (
        ?,
        ?
    )
]]
REPLACE_GROUP_BUY_SQL_PARAM_TYPE = "is"


-------------------------------------------团购狂欢-------------------------------------------

--服务器启动加载全服总数量
function get_server_group_buy(_begin_time, _end_time)
    local group_buy_server = do_get_server_group_buy(_begin_time, _end_time)
    dbserver.remote_call_game("activity_mng.on_get_server_group_buy_from_db", group_buy_server or {})
end

function do_get_server_group_buy(_begin_time, _end_time)

    local time = c_cpu_ms()

    local group_buy_server = {}

    local database = dbserver.g_db_character
    if database:c_stmt_format_query(SELECT_SERVER_GROUP_BUY_SQL, SELECT_SERVER_GROUP_BUY_SQL_PARAM_TYPE, _begin_time, SELECT_SERVER_GROUP_BUY_SQL_RESULT_TYPE ) < 0 then
        c_err("[db_activity](do_get_server_group_buy) c_stmt_format_query on SELECT_SERVER_GROUP_BUY_SQL failed")
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return group_buy_server
    elseif rows >= 1 then
        for i = 1, rows, 1 do
            if database:c_stmt_fetch() == 0 then
                local group_buy = database:c_stmt_get( "group_buy" )
                group_buy = loadstring(group_buy)()

                if group_buy.goods_ and not utils.table.is_empty(group_buy.goods_) then
                    for k,single_group_buy in ipairs(group_buy.goods_) do
                        group_buy_server[k] = group_buy_server[k] or 0
                        group_buy_server[k] = group_buy_server[k] + single_group_buy.buy_count_
                    end
                end
            else
                c_err( "[db_activity](do_get_server_group_buy) c_stmt_fetch failed, index:%d", i)
                return nil
            end
        end
    else
        c_err("[db_activity](do_get_server_group_buy) c_stmt_row_num failed. rows:%s", tostring(rows))
        return nil
    end

    time = c_cpu_ms() - time
    c_prof( "[db_activity](do_get_server_group_buy)rows:%d, use time: %dms", rows, time )

    return group_buy_server

end


function do_get_player_group_buy( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GROUP_BUY_SQL, "i", _player_id, SELECT_GROUP_BUY_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_activity](do_get_player_group_buy) c_stmt_format_query on SELECT_GROUP_BUY_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return utils.serialize_table({})
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            return database:c_stmt_get( "group_buy" )
        end
    end
    return nil
end

function do_save_player_group_buy( _player )
    return dbserver.g_db_character:c_stmt_format_modify(REPLACE_GROUP_BUY_SQL, REPLACE_GROUP_BUY_SQL_PARAM_TYPE, _player.player_id_, _player.group_buy_)
end
