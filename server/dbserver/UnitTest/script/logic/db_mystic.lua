module( "db_mystic", package.seeall )

-- player data process
SELECT_MYSTIC_SQL = [[
    SELECT
    `mystic`  
    from MYSTIC_TBL where player_id = ?;
]]
SELECT_MYSTIC_SQL_RESULT_TYPE = "s"

REPLACE_MYSTIC_SQL = [[
    REPLACE INTO MYSTIC_TBL 
    ( `player_id`, `mystic` ) VALUES
    (
        ?,
        ?
    )
]]
REPLACE_MYSTIC_SQL_PARAM_TYPE = "is"

-- global data process
SELECT_MYSTIC_GLOBAL_SQL = [[
    SELECT
    `dealer_info`
    FROM MYSTIC_GLOBAL_TBL WHERE id = ?;
]]
SELECT_MYSTIC_GLOBAL_RES_TYPE = "s"

REPLACE_MYSTIC_GLOBAL_SQL = [[
    REPLACE INTO MYSTIC_GLOBAL_TBL
    ( `id`, `dealer_info` ) VALUES
    (
        ?,
        ?
    )
]]
REPLACE_MYSTIC_GLOBAL_RES_TYPE = "is"


function do_get_player_mystic( _player_id ) 
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MYSTIC_SQL, "i", _player_id, SELECT_MYSTIC_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_mystic](do_get_player_mystic) c_stmt_format_query on SELECT_MYSTIC_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return init_mystic()
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local mystic_str = database:c_stmt_get( "mystic" )
            return mystic_str 
        end
    end
    return nil
end

function init_mystic()
    local mystic_str = "{}"
    return mystic_str
end

function do_save_player_mystic( _player )
    local player_id = _player.player_id_
    local mystic_str = _player.mystic_str_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_MYSTIC_SQL, REPLACE_MYSTIC_SQL_PARAM_TYPE, player_id, mystic_str )
end

function get_player_mystic( _player_id )
    local player = db_login.g_player_cache:get( _player_id ) 
    if player then
        return player.mystic_ or init_mystic()
    else
        return db_mystic.do_get_player_mystic( _player_id )
    end
end

function do_save_mystic_global( _dealer_info )
    dbserver.g_db_character:c_stmt_format_modify( REPLACE_MYSTIC_GLOBAL_SQL, REPLACE_MYSTIC_GLOBAL_RES_TYPE, 1, _dealer_info )
end

function do_get_mystic_global( )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MYSTIC_GLOBAL_SQL, "i", 1, SELECT_MYSTIC_GLOBAL_RES_TYPE ) < 0 then
        c_err( "[db_mystic](do_get_mystic_global) c_stmt_format_query on  SELECT_MYSTIC_GLOBAL_SQL failed ")
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return init_mystic_global()
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local dealer_str = database:c_stmt_get( "dealer_info" )
            return dealer_str 
        end
    end
    return nil
end

function init_mystic_global()
    return "{}" 
end
