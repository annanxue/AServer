module( "db_mall", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_MALL_STR = ""

SELECT_MALL_SQL = [[
    SELECT
    `mall`
    from MALL_TBL where player_id = ?;
]]

SELECT_MALL_SQL_RESULT_TYPE = "s"

REPLACE_MALL_SQL = [[
    REPLACE INTO MALL_TBL ( `player_id`, `mall` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_MALL_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_mall( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MALL_SQL, "i", _player_id, SELECT_MALL_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_mall](do_get_player_mall) c_stmt_format_query on SELECT_MALL_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_MALL_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local mall_str = database:c_stmt_get( "mall" )
            return mall_str
        end
    end
    return nil
end

function do_save_player_mall( _player )
    local player_id = _player.player_id_
    local mall = _player.mall_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_MALL_SQL, REPLACE_MALL_SQL_PARAM_TYPE, player_id, mall )
    return ret
end

