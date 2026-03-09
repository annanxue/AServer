module( "db_battle_justice", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_BATTLE_JUSTICE_STR = "{}"

SELECT_BATTLE_JUSTICE_SQL = [[
    SELECT
    `battle_justice`
    from BATTLE_JUSTICE_TBL where player_id = ?;
]]

SELECT_BATTLE_JUSTICE_SQL_RESULT_TYPE = "s"

REPLACE_BATTLE_JUSTICE_SQL = [[
    REPLACE INTO BATTLE_JUSTICE_TBL ( `player_id`, `battle_justice` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_BATTLE_JUSTICE_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_battle_justice( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_BATTLE_JUSTICE_SQL, "i", _player_id, SELECT_BATTLE_JUSTICE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_battle_justice](do_get_player_battle_justice) c_stmt_format_query on SELECT_BATTLE_JUSTICE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_BATTLE_JUSTICE_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local battle_justice_str = database:c_stmt_get( "battle_justice" )
            return battle_justice_str
        end
    end
    return nil
end

function do_save_player_battle_justice( _player )
    local player_id = _player.player_id_
    local battle_justice = _player.battle_justice_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_BATTLE_JUSTICE_SQL, REPLACE_BATTLE_JUSTICE_SQL_PARAM_TYPE, player_id, battle_justice )
    return ret
end

