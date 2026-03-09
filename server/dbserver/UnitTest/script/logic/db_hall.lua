module( "db_hall", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
local loadstring = loadstring
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_HALL_STR = ""

SELECT_HALL_SQL = [[
    SELECT
    `hall`
    from HALL_TBL where player_id = ?;
]]

SELECT_HALL_SQL_RESULT_TYPE = "s"

REPLACE_HALL_SQL = [[
    REPLACE INTO HALL_TBL ( `player_id`, `hall` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_HALL_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_hall( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_HALL_SQL, "i", _player_id, SELECT_HALL_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_hall](do_get_player_hall) c_stmt_format_query on SELECT_HALL_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_HALL_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local hall_str = database:c_stmt_get( "hall" )
            return hall_str
        end
    end
    return nil
end

function do_save_player_hall( _player )
    local player_id = _player.player_id_
    local hall = _player.hall_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_HALL_SQL, REPLACE_HALL_SQL_PARAM_TYPE, player_id, hall )
    local func = loadstring( hall )
    if not func then
        c_err("[db_hall](do_save_player_hall) func is nil! player_id:%d, hall_str:%s", player_id, hall or "")
    end
    return ret
end

