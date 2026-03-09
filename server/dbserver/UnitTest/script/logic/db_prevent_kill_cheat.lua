module( "db_prevent_kill_cheat", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_PREVENT_KILL_CHEAT_STR = "do local ret = {} return ret end"

SELECT_PREVENT_KILL_CHEAT_SQL = [[
    SELECT
    `prevent_kill_cheat`
    from PREVENT_KILL_CHEAT_TBL where player_id = ?;
]]

SELECT_PREVENT_KILL_CHEAT_SQL_RESULT_TYPE = "s"

REPLACE_PREVENT_KILL_CHEAT_SQL = [[
    REPLACE INTO PREVENT_KILL_CHEAT_TBL ( `player_id`, `prevent_kill_cheat` ) VALUES 
    (
        ?,
        ?
    )
]]

PREVENT_KILL_CHEAT_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_prevent_kill_cheat( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PREVENT_KILL_CHEAT_SQL, "i", _player_id, SELECT_PREVENT_KILL_CHEAT_SQL_RESULT_TYPE) < 0 then
        c_err( "[db_prevent_kill_cheat](do_get_player_prevent_kill_cheat) c_stmt_format_query on SELECT_PREVENT_KILL_CHEAT_SQLfailed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_PREVENT_KILL_CHEAT_STR 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local prevent_kill_cheat_str = database:c_stmt_get( "prevent_kill_cheat" )
            return prevent_kill_cheat_str 
        end
    end
    return nil
end

function do_save_player_prevent_kill_cheat( _player )
    local player_id = _player.player_id_
    local prevent_kill_cheat = _player.prevent_kill_cheat_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_PREVENT_KILL_CHEAT_SQL, PREVENT_KILL_CHEAT_SQL_PARAM_TYPE, player_id, prevent_kill_cheat )
    return ret
end

