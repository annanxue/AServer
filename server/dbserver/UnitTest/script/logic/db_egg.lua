module( "db_egg", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_EGG_STR = ""

SELECT_EGG_SQL = [[
    SELECT
    `egg`
    from EGG_TBL where player_id = ?;
]]

SELECT_EGG_SQL_RESULT_TYPE = "s"

REPLACE_EGG_SQL = [[
    REPLACE INTO EGG_TBL ( `player_id`, `egg` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_EGG_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_egg( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_EGG_SQL, "i", _player_id, SELECT_EGG_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_egg](do_get_player_egg) c_stmt_format_query on SELECT_EGG_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_EGG_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local egg_str = database:c_stmt_get( "egg" )
            return egg_str
        end
    end
    return nil
end

function do_save_player_egg( _player )
    local player_id = _player.player_id_
    local egg = _player.egg_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_EGG_SQL, REPLACE_EGG_SQL_PARAM_TYPE, player_id, egg )
    return ret
end

