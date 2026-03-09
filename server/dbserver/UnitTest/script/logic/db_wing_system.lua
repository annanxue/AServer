module( "db_wing_system", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_WING_SYSTEM_STR = "{}"

SELECT_WING_SYSTEM_SQL = [[
    SELECT
    `wing_system`
    from WING_SYSTEM_TBL where player_id = ?;
]]

SELECT_WING_SYSTEM_SQL_RESULT_TYPE = "s"

REPLACE_WING_SYSTEM_SQL = [[
    REPLACE INTO WING_SYSTEM_TBL ( `player_id`, `wing_system` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_WING_SYSTEM_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_wing_system( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_WING_SYSTEM_SQL, "i", _player_id, SELECT_WING_SYSTEM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_wing_system](do_get_player_wing_system) c_stmt_format_query on SELECT_WING_SYSTEM_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_WING_SYSTEM_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local wing_system_str = database:c_stmt_get( "wing_system" )
            return wing_system_str
        end
    end
    return nil
end

function do_save_player_wing_system( _player )
    local player_id = _player.player_id_
    local wing_system = _player.wing_system_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_WING_SYSTEM_SQL, REPLACE_WING_SYSTEM_SQL_PARAM_TYPE, player_id, wing_system )
    return ret
end

