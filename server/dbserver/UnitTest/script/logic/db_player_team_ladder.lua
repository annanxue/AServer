module( "db_player_team_ladder", package.seeall )

local default_team_str = "{-1,-1}"
local tinsert = table.insert

SELECT_ALL_PLAYER_TEAM_LADDER_DATA = [[
    SELECT 
    `player_id`,     
    `team_str`
    from PLAYER_TEAM_LADDER_TBL where `team_str` != ?; 
]]

SELECT_ALL_PLAYER_TEAM_LADDER_DATA_RES_TYPE = "is"
 
SELECT_TEAM_LADDER_DATA_SQL = [[
    SELECT
    `team_str`,
    `apply_str`,
    `other_info_str`
    from PLAYER_TEAM_LADDER_TBL where player_id = ?; 
]]

SELECT_TEAM_LADDER_DATA_SQL_RES_TYPE = "sss"

REPLACE_TEAM_LADDER_DATA_SQL = [[
    REPLACE INTO PLAYER_TEAM_LADDER_TBL 
    ( `player_id`, `team_str`, `apply_str`, `other_info_str` ) VALUES
    (
       ?,
       ?,
       ?,
       ?
    )
]]

REPLACE_TEAM_LADDER_DATA_PARAM_TYPE = "isss"

UPDATE_TEAM_LADDER_TEAM_STR_SQL = [[
    UPDATE PLAYER_TEAM_LADDER_TBL set `team_str` = ? where player_id = ?; 
]]

UPDATE_TEAM_LADDER_TEAM_STR_PARAM_TYPE = "si"

local function new_default_team_ladder_data()
    local data =
    {
        team_str_         = default_team_str,
        apply_str_        = "{{},{},{}}", 
        other_info_str_   = "{{},{},{}}", 
    }
    return data
end

function get_player_team_ladder_data( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TEAM_LADDER_DATA_SQL, "i", _player_id, SELECT_TEAM_LADDER_DATA_SQL_RES_TYPE ) < 0 then
        c_err( "[db_player_team_ladder](get_player_team_ladder_data) c_stmt_format_query on SELECT_TEAM_LADDER_DATA_SQL failed, player_id: %d.", _player_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return new_default_team_ladder_data()
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local team_str, apply_str, other_info_str = database:c_stmt_get( "team_str", "apply_str", "other_info_str" )
            return { team_str_ = team_str, apply_str_ = apply_str, other_info_str_ = other_info_str } 
        else
            c_err( "[db_player_team_ladder](get_player_team_ladder_data) c_stmt_fetch failed, player_id: %d", _player_id )
            return nil
        end
    end
end

function save_player_team_ladder_data_all( _player )
    local player_id = _player.player_id_ 
    local data = _player.team_ladder_db_data_
    local team_str = data.team_str_
    local apply_str = data.apply_str_
    local other_info_str = data.other_info_str_

    local database = dbserver.g_db_character
    local ret_code = database:c_stmt_format_modify( REPLACE_TEAM_LADDER_DATA_SQL, REPLACE_TEAM_LADDER_DATA_PARAM_TYPE, 
                                                    player_id, team_str, apply_str, other_info_str )
    if ret_code < 0 then
        c_err( "[db_player_team_ladder](save_player_team_ladder_data_all) update error, player_id: %d, team_str: %s, apply_str: %s, other_info_str: %s", 
                player_id, team_str, apply_str, other_info_str )
    end
    return ret_code 
end

function save_player_team_id( _player_id, _team_str )
    local player = db_login.g_player_cache:get( _player_id )
    if player and player.team_ladder_db_data_ then
        player.team_ladder_db_data_.team_str_ = _team_str
    end

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( UPDATE_TEAM_LADDER_TEAM_STR_SQL, UPDATE_TEAM_LADDER_TEAM_STR_PARAM_TYPE, _team_str, _player_id ) < 0 then
        c_err( "[db_player_team_ladder](save_player_team_id) update data db error! player_id: %d, team_str: %s", _player_id, _team_str )
        return false
    end
    return true
end

local function do_get_all_player_team_ladder_data()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_PLAYER_TEAM_LADDER_DATA, "s", default_team_str, SELECT_ALL_PLAYER_TEAM_LADDER_DATA_RES_TYPE ) < 0 then
        c_err( "[db_ladder_team](do_get_all_player_team_ladder_data) c_stmt_format_query on SELECT_ALL_LADDER_TEAM_DATA_SQL failed" )
        return nil
    end

    local data_list = {}
    local rows = database:c_stmt_row_num() 
    for i = 1, rows do
        if database:c_stmt_fetch() == 0 then
            local player_id, team_str = database:c_stmt_get( "player_id", "team_str" )
            tinsert( data_list, { player_id, team_str } )
        else
            c_err( "[db_ladder_team](do_get_all_player_team_ladder_data) c_stmt_fetch failed, index: %d", i )
            return nil
        end
    end
    return data_list
end

function get_all_player_team_ladder_data()
    local data_list = do_get_all_player_team_ladder_data() or {}
    dbserver.remote_call_game( "event_team_ladder.on_start_get_all_player_team_ladder_data" )

    for i, data in ipairs( data_list ) do
        dbserver.remote_call_game( "event_team_ladder.on_get_all_player_team_ladder_data", data )
    end

    dbserver.remote_call_game( "event_team_ladder.on_finish_get_all_player_team_ladder_data" )
end

