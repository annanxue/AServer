module( "db_god_divination", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_GOD_DIVINATION_STR = "{}"

SELECT_GOD_DIVINATION_SQL = [[
    SELECT
    `god_divination`
    from GOD_DIVINATION_TBL where player_id = ?;
]]

SELECT_GOD_DIVINATION_SQL_RESULT_TYPE = "s"

REPLACE_GOD_DIVINATION_SQL = [[
    REPLACE INTO GOD_DIVINATION_TBL ( `player_id`, `god_divination` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_GOD_DIVINATION_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_god_divination( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GOD_DIVINATION_SQL, "i", _player_id, SELECT_GOD_DIVINATION_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_god_divination](do_get_player_god_divination) c_stmt_format_query on SELECT_GOD_DIVINATION_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_GOD_DIVINATION_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local god_divination_str = database:c_stmt_get( "god_divination" )
            return god_divination_str
        end
    end
    return nil
end

function do_save_player_god_divination( _player )
    local player_id = _player.player_id_
    local god_divination = _player.god_divination_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_GOD_DIVINATION_SQL, REPLACE_GOD_DIVINATION_SQL_PARAM_TYPE, player_id, god_divination )
    return ret
end

