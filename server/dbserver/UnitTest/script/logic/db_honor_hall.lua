module( "db_honor_hall", package.seeall )

local sformat, tinsert, ostime, osdate, tconcat, strsub = string.format, table.insert, os.time, os.date, table.concat, string.sub
local tsort, remote_call_game = table.sort, dbserver.remote_call_game 
local LOG_HEAD = "[db_honor_hall]"

SELECT_HONOR_LADDER_SQL = [[
    SELECT 
            HONOR_LADDER_TBL.`player_id`,
            PLAYER_TBL.`player_name`,
            PLAYER_TBL.`job_id`,
            PLAYER_TBL.`guild_name`,
            `data` 
    FROM    HONOR_LADDER_TBL LEFT JOIN PLAYER_TBL 
    ON      HONOR_LADDER_TBL.player_id = PLAYER_TBL.player_id;
]]

SELECT_HONOR_LADDER_SQL_RESULT_TYPE = "isiss"

TRUNCATE_HONOR_LADDER_SQL = [[
    TRUNCATE HONOR_LADDER_TBL;
]]


REPLACE_HONOR_LADDER_SQL = [[
    REPLACE INTO HONOR_LADDER_TBL ( `player_id`, `data` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_HONOR_LADDER_SQL_PARAM_TYPE = "is"

SELECT_HONOR_HALL_GLOBAL_SQL = [[
    SELECT
    `system_info`
    from HONOR_HALL_GLOBAL_TBL where id = 0;
]]

SELECT_HONOR_HALL_GLOBAL_SQL_RESULT_TYPE = "s"

REPLACE_HONOR_HALL_GLOBAL_SQL = [[
    REPLACE INTO HONOR_HALL_GLOBAL_TBL 
    ( `id`, `system_info` ) VALUES
    (
        0,
        ?
    )
]]

REPLACE_HONOR_HALL_GLOBAL_SQL_PARAM_TYPE = "s"

local function new_player_ladder_dbstr( _player_id, _player_name, _job, _guild_name, _ladder_data_str )
    local data =
    {
        player_id_   = _player_id,
        player_name_ = _player_name,
        job_ = _job, 
        guild_name_ = _guild_name, 
        ladder_data_str_ = _ladder_data_str,
    }
    return data 
end 

function query_all_honor_ladder_data()
    local log_head = sformat( "%s(query_all_honor_ladder_data)", LOG_HEAD )

    local database = dbserver.g_db_character 
    if database:c_stmt_format_query( SELECT_HONOR_LADDER_SQL, "", SELECT_HONOR_LADDER_SQL_RESULT_TYPE ) < 0 then
        c_err( "%s SELECT_HONOR_LADDER_SQL error", log_head )
        return
    end

    local all_ladder_data = {}
    local rows = database:c_stmt_row_num()
    for i = 1, rows do
        if database:c_stmt_fetch() ~= 0 then
            c_err( "%s fetch error", log_head )
            return
        end
        local player_id = database:c_stmt_get( "player_id" )
        local player_name = database:c_stmt_get( "player_name" )
        local job = database:c_stmt_get( "job_id" )
        local guild_name = database:c_stmt_get( "guild_name" )
        local ladder_str = database:c_stmt_get( "data" )
        
        tinsert( all_ladder_data, new_player_ladder_dbstr( player_id, player_name, job, guild_name, ladder_str ) )
    end

    remote_call_game( "honor_ladder_game.on_db_get_player_ladder_data_start" )

    utils.split_task_hanle_ok( remote_call_game,{"honor_ladder_game.on_db_get_player_ladder_data"}, all_ladder_data )

    remote_call_game( "honor_ladder_game.on_db_get_player_ladder_data_finish" )

    c_log( "%s finish, player count: %d", log_head, rows )
end 

function do_save_player_honor_ladder( _player_id, _data_str )
    if dbserver.g_db_character:c_stmt_format_modify( REPLACE_HONOR_LADDER_SQL, REPLACE_HONOR_LADDER_SQL_PARAM_TYPE, _player_id, _data_str ) < 0 then 
        c_err( "[db_honor_hall](do_save_player_honor_ladder) fail, player id: %d", _player_id )
        return
    end 
end

function do_truncate_honor_ladder_tbl()
    if dbserver.g_db_character:c_modify( TRUNCATE_HONOR_LADDER_SQL ) < 0 then 
        c_err( "[db_honor_hall](do_truncate_honor_ladder_tbl) fail" )
        return
    end 
end 

function do_get_honor_hall_global_info()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_HONOR_HALL_GLOBAL_SQL, "", SELECT_HONOR_HALL_GLOBAL_SQL_RESULT_TYPE ) < 0 then
        c_err( "%s c_stmt_format_query on SELECT_HONOR_HALL_GLOBAL_SQL failed", LOG_HEAD )
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

function do_save_honor_hall_global_info( _data_str )
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_HONOR_HALL_GLOBAL_SQL, REPLACE_HONOR_HALL_GLOBAL_SQL_PARAM_TYPE, _data_str )
end

-------------------------------------------------------------------------------------
-- old code to be deleted

function db_save_player_honor_hall_score( _player_honor_hall )
end 

function db_delete_players_honor_hall( _player_list )
end 

function db_zero_today_score( _index )
end 

function db_refresh_get_award_state()
end 

function db_query_top_players_info( _level_player_list )
end 

-------------------------------------------------------------------------------------



