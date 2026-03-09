module( "db_inst", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_INST_STR = "do return {} end"

SELECT_INST_SQL = [[
    SELECT
    `inst`
    from INST_TBL where player_id = ?;
]]

SELECT_INST_SQL_RESULT_TYPE = "s"

REPLACE_INST_SQL = [[
    REPLACE INTO INST_TBL ( `player_id`, `inst` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_INST_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_inst( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_INST_SQL, "i", _player_id, SELECT_INST_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_inst](do_get_player_inst) c_stmt_format_query on SELECT_INST_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_INST_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local inst_str = database:c_stmt_get( "inst" )
            return inst_str
        end
    end
    return nil
end

function do_save_player_inst( _player )
    local player_id = _player.player_id_
    local inst = _player.inst_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_INST_SQL, REPLACE_INST_SQL_PARAM_TYPE, player_id, inst )
    return ret
end

