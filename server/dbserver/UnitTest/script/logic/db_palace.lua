module( "db_palace", package.seeall )

-- player data process
SELECT_PALACE_SQL = [[
    SELECT
    `data`,
    `max_pass_id`,
    `min_cost_time`,
    `pass_timestamp`
    from PALACE_TBL where player_id = ?;
]]
SELECT_PALACE_SQL_RESULT_TYPE = "siii"

SELECT_PALACE_RANK_LIST_SQL = [[
    SELECT
        PLAYER_TBL.`player_id`,
        PLAYER_TBL.`player_name`,
        PALACE_TBL.`max_pass_id`,
        PALACE_TBL.`min_cost_time`,
        PALACE_TBL.`pass_timestamp`,
        PLAYER_TBL.`job_id`,
        PLAYER_TBL.`model_id`,
        PLAYER_TBL.`max_gs`
    FROM PALACE_TBL LEFT JOIN PLAYER_TBL 
        ON PALACE_TBL.player_id = PLAYER_TBL.player_id
    WHERE max_pass_id != 0
        ORDER BY max_pass_id DESC, min_cost_time ASC, pass_timestamp ASC
    LIMIT ]] .. share_const.PALACE_RANK_NUM .. ";"

SELECT_PALACE_RANK_LIST_SQL_RESULT_TYPE = "isiiiiii"

DELETE_PALACE_RANK_LIST_SQL = [[
    UPDATE PALACE_TBL set max_pass_id = 0, min_cost_time = 0, pass_timestamp = 0;
]]

REPLACE_PALACE_SQL = [[
    REPLACE INTO PALACE_TBL 
    ( `player_id`, `data`, `max_pass_id`, `min_cost_time`, `pass_timestamp` ) VALUES
    (
        ?,
        ?,
        ?,
        ?,
        ?
    )
]]
REPLACE_PALACE_SQL_PARAM_TYPE = "isiii"

function do_get_player_palace( _player_id ) 
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PALACE_SQL, "i", _player_id, SELECT_PALACE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_palace](do_get_player_palace) c_stmt_format_query on SELECT_PALACE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return init_palace()
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local palace_str = database:c_stmt_get( "data" )
            local max_pass_id = database:c_stmt_get( "max_pass_id" )
            local min_cost_time = database:c_stmt_get( "min_cost_time" )
            local pass_timestamp = database:c_stmt_get( "pass_timestamp" ) 
            local palace_data = 
            {
                palace_str_ = palace_str,
                max_pass_id_ = max_pass_id,
                min_cost_time_ = min_cost_time,
                pass_timestamp_ = pass_timestamp,
            }
            return palace_data 
        end
    end
    return nil
end

function init_palace()
    local palace_data =
    {
        palace_str_ = "{}",
        max_pass_id_ = 0,
        min_cost_time_ = 0,
        pass_timestamp_ = 0,
    }
    return palace_data 
end

function do_save_player_palace( _player )
    local player_id = _player.player_id_
    local palace_data = _player.palace_data_
    local palace_str = palace_data.palace_str_
    local max_pass_id = palace_data.max_pass_id_
    local min_cost_time = palace_data.min_cost_time_
    local pass_timestamp = palace_data.pass_timestamp_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_PALACE_SQL, REPLACE_PALACE_SQL_PARAM_TYPE, player_id, palace_str, max_pass_id, min_cost_time, pass_timestamp )
end

function init_palace_rank_list()
    local rank_list = do_get_palace_rank_list()
    dbserver.remote_call_game( "palace_rank_mng.init_palace_rank_list", rank_list )
    c_log( "[db_palace](init_palace_rank_list) palace rank list init done" )
end

function do_get_palace_rank_list()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PALACE_RANK_LIST_SQL, "", SELECT_PALACE_RANK_LIST_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_palace](do_get_palace_rank_list) c_stmt_format_query on SELECT_PALACE_RANK_LIST_SQL failed" )
        return {}
    end

    local rank_list = {}
    local rows = database:c_stmt_row_num()
    for i = 1, rows do
        if database:c_stmt_fetch() ~= 0 then
            c_err( "[db_palace](do_get_palace_rank_list) fetch row failed" )
            return {}
        end

        local player_id       = database:c_stmt_get( "player_id" )
        local player_name     = database:c_stmt_get( "player_name" )
        local max_pass_id     = database:c_stmt_get( "max_pass_id" )
        local min_cost_time   = database:c_stmt_get( "min_cost_time" )
        local pass_timestamp  = database:c_stmt_get( "pass_timestamp" )
        local job_id          = database:c_stmt_get( "job_id" )
        local model_id        = database:c_stmt_get( "model_id" )
        local max_gs          = database:c_stmt_get( "max_gs" )

        if not player_id or 
           not player_name or 
           not max_pass_id or 
           not min_cost_time or 
           not pass_timestamp or 
           not job_id or 
           not model_id or 
           not max_gs then
            c_err( "[db_palace](do_get_palace_rank_list) get column failed" )
            return {}
        end

        rank_list[i] =
        {
            player_id_       = player_id,
            player_name_     = player_name,
            max_pass_id_     = max_pass_id,
            min_cost_time_   = min_cost_time,
            pass_timestamp_  = pass_timestamp,
            rank_            = i,
        }
        if i >= 1 and i <= 3 then
            rank_list[i].hall_palace_info_ = {
                server_id_ = 0, -- 先用0，到GS再换掉
                player_id_ = player_id,
                player_name_ = player_name,
                job_id_ = job_id,
                model_id_ = model_id,
                wing_id_ = 0, -- 先用0顶着
                active_weapon_sfxs_ = {}, -- 先用空的,上榜玩家登陆过就会更新
                max_gs_ = max_gs,
            }
        end
    end
    return rank_list
end

function do_delete_palace_rank_list()
    local rows = dbserver.g_db_character:c_stmt_format_modify( DELETE_PALACE_RANK_LIST_SQL, "" )
    if rows < 0 then
        c_err( "[db_palace](do_delete_palace_rank_list) delete palace rank list failed" )
        return
    end
    c_log( "[db_palace](do_delete_palace_rank_list) delete palace rank list, rows: %d", rows )
end