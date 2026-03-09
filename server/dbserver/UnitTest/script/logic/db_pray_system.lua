module( "db_pray_system", package.seeall )

SELECT_PRAY_SYSTEM_SQL = [[
    SELECT
    `system_info`
    from PRAY_SYSTEM_TBL where id = 0;
]]
SELECT_PRAY_SYSTEM_SQL_RESULT_TYPE = "s"

REPLACE_PRAY_SYSTEM_SQL = [[
    REPLACE INTO PRAY_SYSTEM_TBL 
    ( `id`, `system_info` ) VALUES
    (
        0,
        ?
    )
]]
REPLACE_PRAY_SYSTEM_SQL_PARAM_TYPE = "s"

function do_get_pray_system_info()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PRAY_SYSTEM_SQL, "", SELECT_PRAY_SYSTEM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_pray_system] c_stmt_format_query on SELECT_PRAY_SYSTEM_SQL failed" )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return "{}"
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local system_info_str = database:c_stmt_get( "system_info" )
            return system_info_str 
        end
    end
    return nil
end

function do_save_player_pray_system( _pray_system_str )
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_PRAY_SYSTEM_SQL, REPLACE_PRAY_SYSTEM_SQL_PARAM_TYPE, _pray_system_str )
end

