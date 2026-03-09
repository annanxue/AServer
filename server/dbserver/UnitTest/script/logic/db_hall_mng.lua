module( "db_hall_mng", package.seeall )

local os_time, tinsert = os.time, table.insert

local NIL_UPVOTE_DATA_STR = ""

SELECT_ALL_HALLS_SQL = [[
    SELECT 
    `hall_type`, 
    `hall_ver`, 
    `hall_list_str`
    FROM HALL_MNG_TBL;
]]
SELECT_ALL_HALLS_SQL_PARAM_TYPE = ""
SELECT_ALL_HALLS_SQL_RESULT_TYPE = "iis"


REPLACE_HALL_SQL = [[
    REPLACE INTO HALL_MNG_TBL
    ( 
    `hall_type`,
    `hall_ver`,
    `hall_list_str`
    ) 
    VALUES 
    ( ?, ?, ? );
]]

REPLACE_HALL_SQL_PARAM_TYPE = "iis"

SELECT_TOP_N_UPVOTE_DATA_SQL = [[
    SELECT 
    `player_id`, 
    `upvote_data_str`,
    `weekly_count`,
    `reset_time`
    FROM HALL_UPVOTE_TBL WHERE `weekly_count` > 0 ORDER BY `weekly_count` DESC LIMIT ?;
]]

SELECT_TOP_N_UPVOTE_DATA_SQL_PARAM_TYPE = "i"
SELECT_TOP_N_UPVOTE_DATA_SQL_RESULT_TYPE = "isii"

SELECT_UPVOTE_DATA_SQL = [[
    SELECT
    `upvote_data_str`,
    `weekly_count`,
    `reset_time`
    FROM HALL_UPVOTE_TBL WHERE player_id = ?;
]]

SELECT_UPVOTE_DATA_SQL_PARAM_TYPE = "i"
SELECT_UPVOTE_DATA_SQL_RESULT_TYPE = "sii"

REPLACE_UPVOTE_DATA_SQL = [[
    REPLACE INTO HALL_UPVOTE_TBL
    ( 
    `player_id`,
    `upvote_data_str`,
    `weekly_count`,
    `reset_time`
    ) 
    VALUES 
    ( ?, ?, ?, ? );
]]

REPLACE_UPVOTE_DATA_SQL_PARAM_TYPE = "isii"

RESET_UPVOTE_WEEKLY_COUNT_SQL = [[
    UPDATE HALL_UPVOTE_TBL 
    SET `weekly_count` = 0, `reset_time` = ? 
    WHERE reset_time < ?;
]]

RESET_UPVOTE_WEEKLY_COUNT_SQL_PARAM_TYPE = "ii"

-- ------------------------------------
-- Local Functions
-- ------------------------------------
function db_save_hall( _hall_type, _hall_ver, _hall_list_str )
    local time = c_cpu_ms()
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( REPLACE_HALL_SQL, REPLACE_HALL_SQL_PARAM_TYPE,
        _hall_type,
        _hall_ver,
        _hall_list_str ) < 0 then
    c_err( "[db_hall_mng](db_save_hall)failed to save hall, hall_type:%d, hall_ver:%d, hall_list_str:%s", _hall_type, _hall_ver, _hall_list_str )
    end
    time = c_cpu_ms() - time
    c_prof( "[db_hall_mng](db_save_hall) hall_type: %d, use time: %d ms", _hall_type, time )
end

function do_get_all_halls()
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_HALLS_SQL, SELECT_ALL_HALLS_SQL_PARAM_TYPE, SELECT_ALL_HALLS_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_hall_mng](do_get_all_halls)  SELECT_ALL_HALLS_SQL failed" )
        return nil
    end

    local all_halls = {}
    local rows = database:c_stmt_row_num()

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local hall_type, hall_ver, hall_list_str = database:c_stmt_get( "hall_type", "hall_ver", "hall_list_str" )
            tinsert( all_halls, { hall_type, hall_ver, hall_list_str } )
        else
            c_err( "[db_hall_mng](do_get_all_halls) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end

    time = c_cpu_ms() - time
    c_prof( "[db_hall_mng](do_get_all_halls)rows:%d, use time: %dms", rows, time )

    return all_halls
end


function db_load_all_hall_data()
    local all_halls = do_get_all_halls()
    c_assert( all_halls, "[db_hall_mng](db_load_all_hall_data) failed to get hall data! please check")

    for _, raw_data in ipairs( all_halls ) do
        local hall_type = raw_data[1]
        local hall_ver = raw_data[2]
        local hall_list_str = raw_data[3]
        dbserver.remote_call_game( "hall_mng.on_db_load_all_hall_data", hall_type, hall_ver, hall_list_str)
    end

    --[[
    --TODO 感觉不需要了
    dbserver.remote_call_game( "hall_mng.on_db_get_hall_data_finish" )
    --]]
end

local function do_get_top_n_upvote_data( _top_n )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TOP_N_UPVOTE_DATA_SQL, SELECT_TOP_N_UPVOTE_DATA_SQL_PARAM_TYPE, _top_n, SELECT_TOP_N_UPVOTE_DATA_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_hall_mng](do_get_top_n_upvote_data)  SELECT_TOP_N_UPVOTE_DATA_SQL failed" )
        return nil
    end

    local list_data = {}
    local rows = database:c_stmt_row_num()

    if rows > 0 then
        for i = 1, rows, 1 do
            if database:c_stmt_fetch() == 0 then
                local player_id, upvote_data_str, weekly_count, reset_time = database:c_stmt_get( "player_id", "upvote_data_str", "weekly_count", "reset_time" )
                tinsert( list_data, { player_id, upvote_data_str, weekly_count, reset_time } )
            else
                c_err( "[db_hall_mng](do_get_top_n_upvote_data) c_stmt_fetch failed, index:%d", i)
                return nil
            end
        end
    end

    time = c_cpu_ms() - time
    c_prof( "[db_hall_mng](do_get_top_n_upvote_data)rows:%d, use time: %dms", rows, time )

    return list_data

end

function db_reset_upvote_weekly_count( _reset_time )
    local time = c_cpu_ms()
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( RESET_UPVOTE_WEEKLY_COUNT_SQL, RESET_UPVOTE_WEEKLY_COUNT_SQL_PARAM_TYPE,
        _reset_time,
        _reset_time ) < 0 then
    c_err( "[db_hall_mng](db_reset_upvote_weekly_count) failed to reset upvote weekly count, reset_time:%d", _reset_time )
    return false
    end
    time = c_cpu_ms() - time
    c_prof( "[db_hall_mng](db_reset_upvote_weekly_count) reset_time: %d, use time: %d ms", _reset_time, time )
    return true
end

function db_load_top_n_upvote_data( _top_n, _reset_time )

    local is_succ = db_reset_upvote_weekly_count(_reset_time)
    c_assert( is_succ, "[db_hall_mng](db_load_top_n_upvote_data) failed to reset_upvote_weekly_count! please check")

    local list_data = do_get_top_n_upvote_data( _top_n )
    c_assert( list_data, "[db_hall_mng](db_load_top_n_upvote_data) failed to get hall upvote data! please check")

    for _, raw_data in ipairs( list_data ) do
        local player_id = raw_data[1]
        local upvote_data_str = raw_data[2]
        local weekly_count = raw_data[3]
        local reset_time = raw_data[4]
        dbserver.remote_call_game( "hall_mng.on_db_load_top_n_upvote_data", player_id, upvote_data_str, weekly_count, reset_time  )
    end
    
    local loaded_count = #list_data
    dbserver.remote_call_game( "hall_mng.on_db_load_top_n_upvote_data_finish", _top_n, loaded_count )
end

local function do_get_upvote_data( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_UPVOTE_DATA_SQL, SELECT_UPVOTE_DATA_SQL_PARAM_TYPE, _player_id, SELECT_UPVOTE_DATA_SQL_RESULT_TYPE) < 0 then
        c_err( "[db_hall_mng](do_get_upvote_data) c_stmt_format_query on SELECT_UPVOTE_DATA_SQL failed, player id: %d", _player_id )
        return nil, 0, 0
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return NIL_UPVOTE_DATA_STR, 0, 0
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local upvote_data_str, weekly_count, reset_time = database:c_stmt_get( "upvote_data_str", "weekly_count", "reset_time" )
            return upvote_data_str, weekly_count, reset_time
        end
    end
    return nil, 0, 0
end


function db_load_upvote_data( _zone_id, _hall_type, _server_id, _player_id, _ranking, _is_new_get_on )
    local upvote_data_str, weekly_count, reset_time = do_get_upvote_data( _player_id )
    dbserver.remote_call_game( "hall_mng.on_db_load_upvote_data", _zone_id, _hall_type, _server_id, _player_id, _ranking, _is_new_get_on, upvote_data_str, weekly_count, reset_time )
end

function db_save_upvote_data( _server_id, _player_id, _upvote_data_str, _weekly_count, _reset_time )
    local time = c_cpu_ms()
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( REPLACE_UPVOTE_DATA_SQL, REPLACE_UPVOTE_DATA_SQL_PARAM_TYPE,
        _player_id,
        _upvote_data_str, _weekly_count, _reset_time ) < 0 then
    c_err( "[db_hall_mng](db_save_upvote_data)failed to save upvote_data, player_id:%d, upvote_data_str:%s, weekly_count:%d, reset_time:%d", _player_id or 0, _upvote_data_str or "", _weekly_count or 0, _reset_time or 0 )

    end
    time = c_cpu_ms() - time
    c_prof( "[db_hall_mng](db_save_upvote_data) player_id: %d, use time: %d ms", _player_id, time )

end

function db_get_hall_upvote_data_for_rank( _server_id, _player_id )
    local upvote_data_str, weekly_count, reset_time = do_get_upvote_data( _player_id )
    dbserver.remote_call_game( "hall_mng.on_db_get_hall_upvote_data_for_rank", _server_id, _player_id, upvote_data_str, weekly_count, reset_time )
end
