module( "db_time_event", package.seeall )

-- ------------------------------------
-- Local Lib Functions
-- ------------------------------------
local t_insert, os_date = table.insert, os.date

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------

local SELECT_ALL_TIME_EVENT_SQL = [[
    SELECT
    `time_event_id`,
    `last_start_time`
    from TIME_EVENT_TBL;
]]

local SELECT_ALL_TIME_EVENT_RESULT_TYPE = "ii"

local INSERT_TIME_EVENT_SQL = [[
    INSERT INTO TIME_EVENT_TBL
    ( `time_event_id`, `last_start_time` ) VALUES 
    ( ?, ? );
]]

local INSERT_TIME_EVENT_PARAM_TYPE = "ii"


local UPDATE_TIME_EVENT_SQL = [[
    UPDATE TIME_EVENT_TBL SET
    last_start_time = ? 
    WHERE time_event_id = ?;    
]]

local UPDATE_TIME_EVENT_PARAM_TYPE = "ii"

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function query_all_time_event_records()
    c_log( "[db_time_event](query_all_time_event_records) begin" )

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_TIME_EVENT_SQL, "", SELECT_ALL_TIME_EVENT_RESULT_TYPE ) < 0 then
        c_err( "[db_time_event](query_all_time_event_records) SELECT_ALL_TIME_EVENT_SQL error" )
        return
    end

    local rows = database:c_stmt_row_num()
    local time_event_list = {}    
    c_log( "[db_time_event](query_all_time_event_records) row: %d", rows )
    for i = 1, rows do
        if database:c_stmt_fetch() == 0 then
            local time_event_id, last_start_time = database:c_stmt_get( "time_event_id", "last_start_time" )  
            local time_event = { id_ = time_event_id, start_time_ = last_start_time }
            t_insert( time_event_list, time_event ) 
        else
            c_err( "[db_time_event](query_all_time_event_records) fetch error row: %d", i )
        end
    end

    dbserver.remote_call_game( "time_event.on_query_database_finish", time_event_list )

    c_log( "[db_time_event](query_all_time_event_records) query end, count: %d", rows )
end

function create_time_event_record( _time_event_id, _start_time )
    dbserver.g_db_character:c_stmt_format_modify( INSERT_TIME_EVENT_SQL, INSERT_TIME_EVENT_PARAM_TYPE, _time_event_id, _start_time )
    c_log( "[db_time_event](create_time_event_record) time_event_id: %d, last_start_time: %s", _time_event_id, os_date( "%c", _start_time ) )
end

function update_time_event_record( _time_event_id, _start_time )
    dbserver.g_db_character:c_stmt_format_modify( UPDATE_TIME_EVENT_SQL, UPDATE_TIME_EVENT_PARAM_TYPE, _start_time, _time_event_id )
    c_log( "[db_time_event](update_time_event_record) time_event_id: %d, last_start_time: %s", _time_event_id, os_date( "%c", _start_time ) )
end

-- ------------------------------------
-- Unit TestCases
-- ------------------------------------

if g_debug then

end
