module( "db_truncheon", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_TRUNCHEON_STR = "do return {} end"

SELECT_TRUNCHEON_SQL = [[
    SELECT
    `truncheon`
    from TRUNCHEON_TBL where player_id = ?;
]]

SELECT_TRUNCHEON_SQL_RESULT_TYPE = "s"

REPLACE_TRUNCHEON_SQL = [[
    REPLACE INTO TRUNCHEON_TBL ( `player_id`, `truncheon` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_TRUNCHEON_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_truncheon( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TRUNCHEON_SQL, "i", _player_id, SELECT_TRUNCHEON_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_truncheon](do_get_player_truncheon) c_stmt_format_query on SELECT_TRUNCHEON_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_TRUNCHEON_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local truncheon_str = database:c_stmt_get( "truncheon" )
            return truncheon_str
        end
    end
    return nil
end

function do_save_player_truncheon( _player )
    local player_id = _player.player_id_
    local truncheon = _player.truncheon_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_TRUNCHEON_SQL, REPLACE_TRUNCHEON_SQL_PARAM_TYPE, player_id, truncheon )
    return ret
end

