module( "db_red", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_RED_STR = ""

SELECT_RED_SQL = [[
    SELECT
    `red`
    from RED_TBL where player_id = ?;
]]

SELECT_RED_SQL_RESULT_TYPE = "s"

REPLACE_RED_SQL = [[
    REPLACE INTO RED_TBL ( `player_id`, `red` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_RED_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_red( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_RED_SQL, "i", _player_id, SELECT_RED_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_red](do_get_player_red) c_stmt_format_query on SELECT_RED_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_RED_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local red_str = database:c_stmt_get( "red" )
            return red_str
        end
    end
    return nil
end

function do_save_player_red( _player )
    local player_id = _player.player_id_
    local red = _player.red_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_RED_SQL, REPLACE_RED_SQL_PARAM_TYPE, player_id, red )
    return ret
end

