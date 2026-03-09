module( "db_small_data", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_SMALL_DATA_STR = "do return {} end"

SELECT_SMALL_DATA_SQL = [[
    SELECT
    `small_data`
    from SMALL_DATA_TBL where player_id = ?;
]]

SELECT_SMALL_DATA_SQL_RESULT_TYPE = "s"

REPLACE_SMALL_DATA_SQL = [[
    REPLACE INTO SMALL_DATA_TBL ( `player_id`, `small_data` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_SMALL_DATA_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_small_data( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SMALL_DATA_SQL, "i", _player_id, SELECT_SMALL_DATA_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_small_data](do_get_player_small_data) c_stmt_format_query on SELECT_SMALL_DATA_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SMALL_DATA_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local small_data_str = database:c_stmt_get( "small_data" )
            return small_data_str
        end
    end
    return nil
end

function do_save_player_small_data( _player )
    local player_id = _player.player_id_
    local small_data = _player.small_data_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_SMALL_DATA_SQL, REPLACE_SMALL_DATA_SQL_PARAM_TYPE, player_id, small_data )
    return ret
end

