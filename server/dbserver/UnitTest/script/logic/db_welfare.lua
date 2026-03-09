module( "db_welfare", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_WELFARE_STR = ""

SELECT_WELFARE_SQL = [[
    SELECT
    `welfare`
    from WELFARE_TBL where player_id = ?;
]]

SELECT_WELFARE_SQL_RESULT_TYPE = "s"

REPLACE_WELFARE_SQL = [[
    REPLACE INTO WELFARE_TBL ( `player_id`, `welfare` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_WELFARE_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_welfare( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_WELFARE_SQL, "i", _player_id, SELECT_WELFARE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_welfare](do_get_player_welfare) c_stmt_format_query on SELECT_WELFARE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_WELFARE_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local welfare_str = database:c_stmt_get( "welfare" )
            return welfare_str
        end
    end
    return nil
end

function do_save_player_welfare( _player )
    local player_id = _player.player_id_
    local welfare = _player.welfare_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_WELFARE_SQL, REPLACE_WELFARE_SQL_PARAM_TYPE, player_id, welfare )
    return ret
end

