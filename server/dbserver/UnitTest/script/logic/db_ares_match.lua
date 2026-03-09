module( "db_ares_match", package.seeall )

-- ------------------------------------
-- Local Lib Functions
-- ------------------------------------

local SELECT_NEWEST_MATCHES_SQL = [[
    SELECT 
    `version`,
    `matches_str`
    FROM ARES_MATCH_TBL 
    ORDER BY `version` DESC 
    LIMIT 1; 
]]

local SELECT_NEWEST_MATCHES_RESULT_TYPE = "is"

local INSERT_MATCHES_SQL = [[
    INSERT INTO ARES_MATCH_TBL 
    ( `version`, `matches_str` ) VALUES
    ( ?, ? );
]]

local INSERT_MATCHES_PARAM_TYPE = "is"

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function get_newest_ares_matches()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_NEWEST_MATCHES_SQL, "", SELECT_NEWEST_MATCHES_RESULT_TYPE ) < 0 then
        c_err( "[db_ares_match](get_newest_ares_matches)c_stmt_format_query on SELECT_NEWEST_MATCHES_SQL failed" )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        local version = 0
        local matches_str = "do local ret={} return ret end" 
        dbserver.remote_call_game( "ares_witness_game.load_ares_matches_from_db", version, matches_str )
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local version = database:c_stmt_get( "version" ) 
            local matches_str = database:c_stmt_get( "matches_str" )
            dbserver.remote_call_game( "ares_witness_game.load_ares_matches_from_db", version, matches_str )
        end
    end
    c_log( "[db_ares_match](get_newest_ares_matches) rows: %d", rows )
end

function save_ares_matches( _version, _matches_str )
    c_log( "[db_ares_match](save_ares_matches) version: %d, matches_str: %s", _version, _matches_str )
    local database = dbserver.g_db_character
    return database:c_stmt_format_modify( INSERT_MATCHES_SQL, INSERT_MATCHES_PARAM_TYPE, _version, _matches_str )
end
