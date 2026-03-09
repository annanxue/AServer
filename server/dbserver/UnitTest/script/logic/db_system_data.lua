module( "db_system_data", package.seeall )

local sformat = string.format

DEFAULT_SYSTEM_DATA_STR = "{}"

SELECT_SYSTEM_DATA_SQL = [[
    SELECT
    `data`
    from %s where player_id = ?;
]]

SELECT_SYSTEM_DATA_SQL_RESULT_TYPE = "s"

REPLACE_SYSTEM_DATA_SQL = [[
    REPLACE INTO %s ( `player_id`, `data` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_SYSTEM_DATA_SQL_PARAM_TYPE = "is"

function do_get_player_system_data( _player_id, _maker )
    local database = dbserver.g_db_character
    local sql_table_name = _maker.sql_table_name
    local query_sql = sformat( SELECT_SYSTEM_DATA_SQL, sql_table_name )
    if database:c_stmt_format_query( query_sql, "i",  _player_id, SELECT_SYSTEM_DATA_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_system_data](do_get_player_system_data) failed, player id: %d, table name: %s", _player_id, sql_table_name )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SYSTEM_DATA_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local str = database:c_stmt_get( "data" )
            return str
        end
    end
    return nil
end

function do_save_player_system_data( _player, _maker )
    local sql_table_name = _maker.sql_table_name
    local query_sql = sformat( REPLACE_SYSTEM_DATA_SQL, sql_table_name )
    local player_id = _player.player_id_
    local save_str = player_t.get_player_system_data( _player, _maker )
    return dbserver.g_db_character:c_stmt_format_modify( query_sql, REPLACE_SYSTEM_DATA_SQL_PARAM_TYPE, player_id, save_str )
end

