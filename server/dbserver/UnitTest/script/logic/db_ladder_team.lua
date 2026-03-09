module( "db_ladder_team", package.seeall )

local tisempty, tinsert, sformat = utils.table.is_empty, table.insert, string.format

SELECT_ALL_LADDER_TEAM_DATA_SQL = [[
    SELECT `team_id`, `battle_str`, `attr_str` from LADDER_TEAM_TBL; 
]]

SELECT_ALL_LADDER_TEAM_DATA_SQL_RES_TYPE = "iss"

INSERT_LADDER_TEAM_SQL = [[
    INSERT INTO LADDER_TEAM_TBL 
    ( `battle_str`, `attr_str` ) VALUES 
    ( 
        ?,
        ?
    );
]]

INSERT_LADDER_TEAM_SQL_PARAM_TYPE = "ss"

REPLACE_LADDER_TEAM_DATA_SQL = [[
    REPLACE INTO LADDER_TEAM_TBL
    ( `team_id`, `battle_str`, `attr_str` ) VALUES
    (
       ?,
       ?,
       ?
    );
]]

REPLACE_LADDER_TEAM_DATA_SQL_PARAM_TYPE = "iss"

DELETE_LADDER_TEAM_DATA_SQL = [[
    DELETE FROM LADDER_TEAM_TBL WHERE team_id = ?;
]]

DELETE_LADDER_TEAM_DATA_SQL_PARAM_TYPE = "i"

CLEAR_ALL_TEAM_SEASON_DATA_SQL = [[
    UPDATE LADDER_TEAM_TBL set `battle_str` = ?; 
]]

CLEAR_ALL_TEAM_SEASON_DATA_PARAM_TYPE = "s" 

local function do_get_all_team_data()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_LADDER_TEAM_DATA_SQL, "", SELECT_ALL_LADDER_TEAM_DATA_SQL_RES_TYPE ) < 0 then
        c_err( "[db_ladder_team](do_get_all_team_data) c_stmt_format_query on SELECT_ALL_LADDER_TEAM_DATA_SQL failed" )
        return nil
    end

    local data_list = {}
    local rows = database:c_stmt_row_num() 
    for i = 1, rows do
        if database:c_stmt_fetch() == 0 then
            local team_id, battle_str, attr_str = database:c_stmt_get( "team_id", "battle_str", "attr_str" )
            tinsert( data_list, { team_id, battle_str, attr_str } )
        else
            c_err( "[db_ladder_team](do_get_all_team_data) c_stmt_fetch failed, index: %d", i )
            return nil
        end
    end
    return data_list
end

function get_all_team_data()
    local data_list = do_get_all_team_data() or {}
    dbserver.remote_call_game( "event_team_ladder.on_start_get_team_db_data" )

    for i, data in ipairs( data_list ) do
        dbserver.remote_call_game( "event_team_ladder.on_get_team_db_data", data )
    end

    dbserver.remote_call_game( "event_team_ladder.on_finish_get_team_db_data" )
end

function create_team( _player_id, _battle_str, _attr_str )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( INSERT_LADDER_TEAM_SQL, INSERT_LADDER_TEAM_SQL_PARAM_TYPE, _battle_str, _attr_str ) < 0 then
        c_err( "[db_ladder_team](create_team) failed to insert team, player_id: %d", _player_id )
        dbserver.remote_call_game( "event_team_ladder.on_db_create_team_result", _player_id, -1, "", "" )
        return
    end

    local team_id = database:c_stmt_get_insert_id()
    dbserver.remote_call_game( "event_team_ladder.on_db_create_team_result", _player_id, team_id, _battle_str, _attr_str )
    c_log( "[db_ladder_team](create_team) create team succ, player_id: %d, team_id: %d", _player_id, team_id )
end

function save_team_data( _team_id, _battle_str, _attr_str )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( REPLACE_LADDER_TEAM_DATA_SQL, REPLACE_LADDER_TEAM_DATA_SQL_PARAM_TYPE, _team_id, _battle_str, _attr_str ) < 0 then
        c_err( "[db_ladder_team](save_team_data)update data db error! team_id: %d, battle_str: %s, attr_str: %s", _team_id, _battle_str, _attr_str )
        return false
    end
    return true
end

function delete_team_data( _team_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( DELETE_LADDER_TEAM_DATA_SQL, DELETE_LADDER_TEAM_DATA_SQL_PARAM_TYPE, _team_id ) < 0 then
        c_err( "[db_ladder_team](delete_team_data)delete data db error! team_id: %d", _team_id )
        return false
    end
    return true
end

function clear_all_team_season_data( _default_str )
    if dbserver.g_db_character:c_stmt_format_modify( CLEAR_ALL_TEAM_SEASON_DATA_SQL, CLEAR_ALL_TEAM_SEASON_DATA_PARAM_TYPE, _default_str ) < 0 then
        c_err( "[db_player_team_ladder](clear_all_team_season_data) update data db error!" )
        return false
    end
    return true
end

