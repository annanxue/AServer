module( "db_festival", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_FESTIVAL_STR = ""

SELECT_FESTIVAL_SQL = [[
    SELECT
    `festival`
    from FESTIVAL_TBL where player_id = ?;
]]

SELECT_FESTIVAL_SQL_RESULT_TYPE = "s"

REPLACE_FESTIVAL_SQL = [[
    REPLACE INTO FESTIVAL_TBL ( `player_id`, `festival` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_FESTIVAL_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_festival( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_FESTIVAL_SQL, "i", _player_id, SELECT_FESTIVAL_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_festival](do_get_player_festival) c_stmt_format_query on SELECT_FESTIVAL_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_FESTIVAL_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local festival_str = database:c_stmt_get( "festival" )
            return festival_str
        end
    end
    return nil
end

function do_save_player_festival( _player )
    local player_id = _player.player_id_
    local festival = _player.festival_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_FESTIVAL_SQL, REPLACE_FESTIVAL_SQL_PARAM_TYPE, player_id, festival )
    return ret
end

