module( "db_attend", package.seeall )
-------------------------------------------------
-- 玩家签到奖励和等级奖励数据库处理
-------------------------------------------------
--
local ostime, diff_day, tinsert = os.time, utils.diff_day, table.insert

SELECT_ATTEND_SQL = [[
    SELECT
    `data`  
    from PLAYER_ATTEND_TBL where player_id = ?;
]]
SELECT_ATTEND_SQL_RESULT_TYPE = "s"

REPLACE_ATTEND_SQL = [[
    INSERT INTO PLAYER_ATTEND_TBL 
    ( `player_id`, `data` ) VALUES
    (
        ?,
        ?
    )
    ON DUPLICATE KEY UPDATE 
    data = VALUES(data);
]]
REPLACE_ATTEND_SQL_PARAM_TYPE = "is"

SELECT_ATTEND_PUSH_INFO = [[
   SELECT
         PLAYER_ATTEND_TBL.player_id,
         UNIX_TIMESTAMP( PLAYER_TBL.`create_time` ) as c_time,
         PLAYER_TBL.`mipush_regid`,
         PLAYER_ATTEND_TBL.`data`
    FROM PLAYER_ATTEND_TBL 
    LEFT JOIN PLAYER_TBL
    ON PLAYER_ATTEND_TBL.player_id = PLAYER_TBL.player_id
    WHERE create_time > 0 and PLAYER_TBL.`mipush_regid` <> '';
]]

function do_get_player_attend( _player_id ) 
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ATTEND_SQL, "i", _player_id, SELECT_ATTEND_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_attend](do_get_player_attend) c_stmt_format_query on SELECT_ATTEND_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return init_attend()
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local attend_str = database:c_stmt_get( "data" )
            return attend_str 
        end
    end
    return nil
end

function init_attend()
    return "{}" 
end

function do_save_player_attend( _player )
    local player_id = _player.player_id_
    local attend_str = _player.attend_str_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_ATTEND_SQL, REPLACE_ATTEND_SQL_PARAM_TYPE, player_id, attend_str )
end

function get_player_attend( _player_id )
    local player = db_login.g_player_cache:get( _player_id ) 
    if player then
        return player.attend_ or init_attend()
    else
        return db_attend.do_get_player_attend( _player_id )
    end
end

function get_attend_push_data()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ATTEND_PUSH_INFO, "", "iiss" ) < 0 then
        c_err( "[db_attend](get_attend_push_data) c_stmt_format_query on SELECT_ATTEND_PUSH_INFO faild, %s", SELECT_ATTEND_PUSH_INFO )
        return nil
    end

    local total_data = {}

    local rows = database:c_stmt_row_num()
    for i = 1, rows do
        if database:c_stmt_fetch() ~= 0 then
            c_err( "[db_attend](get_attend_push_data) fetch error" )
        end

        local player_id = database:c_stmt_get( "player_id" )
        local create_time = database:c_stmt_get( "c_time" )
        local mipush_regid = database:c_stmt_get( "mipush_regid" )
        local attend_data = database:c_stmt_get("data")
        local now_time = ostime()

        local days = diff_day( now_time, create_time )
        if days < 7 then
            local attend_tbl = loadstring( "return" .. attend_data )()
            tinsert( total_data, { player_id, create_time, mipush_regid, attend_tbl[1] } )
        end
    end

    dbserver.split_remote_call_game( "attend_push_mng.on_db_get_attend_push_data", total_data )
    c_log( "[db_attend](get_attend_push_data) get attend push times succ, count:%d", #total_data )
end

