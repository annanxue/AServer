module( "db_sanding", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_SANDING_STR = ""

SELECT_SANDING_SQL = [[
    SELECT
    `sanding`
    from SANDING_TBL where player_id = ?;
]]

SELECT_SANDING_SQL_RESULT_TYPE = "s"

REPLACE_SANDING_SQL = [[
    REPLACE INTO SANDING_TBL ( `player_id`, `sanding` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_SANDING_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_sanding( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SANDING_SQL, "i", _player_id, SELECT_SANDING_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_sanding](do_get_player_sanding) c_stmt_format_query on SELECT_SANDING_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SANDING_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local sanding_str = database:c_stmt_get( "sanding" )
            return sanding_str
        end
    end
    return nil
end

function do_save_player_sanding( _player )
    local player_id = _player.player_id_
    local sanding = _player.sanding_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_SANDING_SQL, REPLACE_SANDING_SQL_PARAM_TYPE, player_id, sanding )
    return ret
end

