module( "db_time", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------

local SELECT_TIME_SQL = [[
    SELECT
    `time_str`
    from PLAYER_TIME_TBL where player_id = ?;
]]

local SELECT_TIME_RESULT_TYPE = "s"

local MODIFY_TIME_SQL = [[
    REPLACE INTO PLAYER_TIME_TBL 
    ( `player_id`, `time_str` ) VALUES ( ?, ? );
]]

local MODIFY_TIME_PARAM_TYPE = "is"

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_time( _player_id )
    local database = dbserver.g_db_character 
    if database:c_stmt_format_query( SELECT_TIME_SQL, "i", _player_id, SELECT_TIME_RESULT_TYPE ) < 0 then
        c_err( "[db_time](do_get_player_time) c_stmt_format_query on SELECT_TIME_SQL failed, player_id: %d", _player_id )
        return 
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { time_str_ = "" }
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local time_str = database:c_stmt_get( "time_str" )
            return { time_str_ = time_str }
        end
    end
end

function do_save_player_time( _player )
    local player_id = _player.player_id_
    local time = _player.time_
    local time_str = time.time_str_
    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_TIME_SQL, MODIFY_TIME_PARAM_TYPE, player_id, time_str )
end
