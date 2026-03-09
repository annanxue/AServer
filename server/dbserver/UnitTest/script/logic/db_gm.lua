module( "db_gm", package.seeall )

local sformat = string.format

SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL = [[
    SELECT player_id FROM PLAYER_TBL WHERE player_name = ?;
]]
SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL_PARAM_TYPE = "s"
SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL_RESULT_TYPE = "i"

SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL = [[
    SELECT machine_code_id AS account_id FROM PLAYER_TBL WHERE player_id = ?;
]]
SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL_PARAM_TYPE = "i"
SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL_RESULT_TYPE = "i"



SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL = [[
    SELECT player_id, player_name FROM PLAYER_TBL WHERE player_name like ?;
]]
SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL_PARAM_TYPE = "s"
SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL_RESULT_TYPE = "is"



SELECT_PLAYER_BATCH_SQL = [[
    SELECT
    `player_id`,
    `player_name`, 
    `level`, 
    `job_id`
    from PLAYER_TBL where player_id in(%s);
]]

SELECT_PLAYER_BATCH_SQL_PARAM_TYPE = ""
SELECT_PLAYER_BATCH_SQL_RESULT_TYPE = "isii"

function query_player_info( _browser_client_id, _player_id )
    if not (_player_id > 0) then
        local err_msg = sformat( "[db_gm](query_player_info) failed to query player info, "..
                                 "player_id: %d, join_retcode: %d", _player_id, login_code.PLAYER_JOIN_SQL_ERR )
        
        gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
        c_err(err_msg)

        return
    end
    local join_retcode, player = db_login.do_player_join( _player_id )
    if not player then
        local err_msg = sformat( "[db_gm](query_player_info) failed to query player info, "..
                                 "player_id: %d, join_retcode: %d", _player_id, join_retcode )
        gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
        return
    end

    dbserver.remote_call_game( "dbclient.on_gm_query_player_info", _browser_client_id, player )
    c_gm( "[db_gm](query_player_info) player_id: %d", _player_id )
end

function query_player_info_by_player_name( _browser_client_id, _player_name )
    local database = dbserver.g_db_character 

    if database:c_stmt_format_query( SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL
                                   , SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL_PARAM_TYPE
                                   , _player_name
                                   , SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL_RESULT_TYPE
                                   ) < 0 then
        local err_msg = "[db_gm](query_player_info_by_player_name) SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL error"
        gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
        c_err( err_msg )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows < 1 then
        local err_msg = sformat( "[db_gm](query_player_info_by_player_name) no player named '%s'", _player_name )
        gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
        c_err( err_msg )
        return
    end

    if database:c_stmt_fetch() ~= 0 then
        local err_msg = "[db_gm](query_player_info_by_player_name) SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL fetch error "
        gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
        c_err( err_msg )
        return
    end

    local player_id = database:c_stmt_get( "player_id" )

    dbserver.remote_call_game( "gm_cmd.query_player_info", 0, _browser_client_id, tostring( player_id ) )

    c_gm( "[db_gm](query_player_info_by_player_name) player_id: %d, player_name: '%s'", player_id, _player_name )
end

function forbid_accounts_by_player_ids( _browser_client_id, _player_forbid_tbl, _is_forbid_machine )
    local account_ids = ""
    local timestamps = ""
    local account_ids_empty = true
    local player_cnt = 0
    local database = dbserver.g_db_character 

    for player_id, forbid_end_time in pairs( _player_forbid_tbl ) do
        if database:c_stmt_format_query( SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL
                                       , SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL_PARAM_TYPE
                                       , player_id
                                       , SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL_RESULT_TYPE
                                       ) < 0 then
            local err_msg = sformat( "[db_gm](forbid_accounts_by_player_ids) SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL error, player_id: %d", player_id )
            gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
            c_err( err_msg )
            return
        end

        local rows = database:c_stmt_row_num()
        if rows == 1 then
            if database:c_stmt_fetch() ~= 0 then
                local err_msg = sformat( "[db_gm](forbid_accounts_by_player_ids) SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL fetch error, player_id: %d", player_id )
                gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
                c_err( err_msg )
                return
            end
        end

        local account_id = database:c_stmt_get( "account_id" )
        if not account_ids_empty then
            account_ids = account_ids .. ","
            timestamps = timestamps .. ","
        end

        account_ids = account_ids .. tostring( account_id )
        timestamps = timestamps .. tostring( forbid_end_time )
        account_ids_empty = false
        player_cnt = player_cnt + 1
        c_gm( "[db_gm](forbid_accounts_by_player_ids) browser_client_id: %d, player_id: %d, account_id: %d, forbid_end_time: %d"
            , _browser_client_id, player_id, account_id, forbid_end_time )
    end

    dbserver.remote_call_game( "dbclient.on_get_account_ids_to_forbid_accounts", _browser_client_id, account_ids, timestamps, _is_forbid_machine )
    c_gm( "[db_gm](forbid_accounts_by_player_ids) browser_client_id: %d, # of players: %d", _browser_client_id, player_cnt )
end

function gmsvr_query_player_info( _dpid, _player_id )
    local ret, player = db_login.do_player_join( _player_id )
    if not player then
        dbserver.remote_call_game( "gmsvr.on_query_player_info_callback" , _dpid, nil )
        return
    end

    local game_account_info = db_login.do_get_account_info(player.account_id_)
    if not game_account_info then
        dbserver.remote_call_game( "gmsvr.on_query_player_info_callback" , _dpid, nil )
        return
    end

    player.game_account_info_ = game_account_info

    dbserver.remote_call_game( "dbclient.on_gmsvr_query_player_info", _dpid, player )
    c_gm( "[db_gm](gmsvr_query_player_info) player_id: %d", _player_id )
end

function gmsvr_query_player_info_batch( _callback, _param )
    local player_list = query_player_batch_new( _param.player_id_list_str_, _param.server_id_ )
    -- if not player_list then
    --     dbserver.remote_call_game(_callback , _param, nil)
    --     return
    -- end

    dbserver.remote_call_game(_callback, _param, player_list)
end

function gmsvr_query_player_info_by_player_name( _dpid, _player_name )
    local database = dbserver.g_db_character 
    if database:c_stmt_format_query( SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL
                                   , SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL_PARAM_TYPE
                                   , _player_name
                                   , SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL_RESULT_TYPE
                                   ) < 0 then
        dbserver.remote_call_game( "gmsvr.on_query_player_info_callback" , _dpid, nil )
        c_err( "[db_gm](gmsvr_query_player_info_by_player_name) SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL error"  )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows < 1 then
        dbserver.remote_call_game( "gmsvr.on_query_player_info_callback" , _dpid, nil )
        c_err(  "[db_gm](query_player_info_by_player_name) no player named '%s'", _player_name )
        return
    end

    if database:c_stmt_fetch() ~= 0 then
        dbserver.remote_call_game( "gmsvr.on_query_player_info_callback" , _dpid, nil )
        c_err( "[db_gm](query_player_info_by_player_name) SELECT_PLAYER_ID_BY_PLAYER_NAME_SQL fetch error " )
        return
    end

    local player_id = database:c_stmt_get( "player_id" )
    gmsvr_query_player_info(_dpid, player_id)
    c_gm( "[db_gm](gm_svr_query_player_info_by_player_name) player_id: %d, player_name: '%s'", player_id, _player_name )
end

function query_player_account(_player_id)
    local database = dbserver.g_db_character 
    if database:c_stmt_format_query( 
        SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL, 
        SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL_PARAM_TYPE, 
        _player_id, 
        SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL_RESULT_TYPE) < 0 then

        c_err( sformat( "[db_gm](query_player_account) SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL error, player_id: %d", _player_id ) )
        return nil 
    end

    local rows = database:c_stmt_row_num()
    if rows == 1 then
        if database:c_stmt_fetch() ~= 0 then
            c_err( sformat( "[db_gm](query_player_account) SELECT_ACCOUNT_ID_BY_PLAYER_ID_SQL fetch error, player_id: %d", _player_id ) )
            return nil
        end

        local account_id = database:c_stmt_get( "account_id" )
        return account_id
    end

    return nil 
end

function gmsvr_forbid_accounts_by_player_ids( _dpid, _player_list, _time_list, _is_forbid_machine )
    local database = dbserver.g_db_character 
    local fail_player_id_list = {}
    local account_list =  {} 
    local time_list = {}
    local player_id_str = ""
    local idx = 0
    for _, player_id in ipairs( _player_list ) do
        idx = idx + 1
        player_id_str = player_id_str ..","..player_id
        local account_id = query_player_account(player_id)
        if not account_id then
            table.insert(fail_player_id_list, player_id)
        else
            table.insert(account_list, account_id)
            table.insert(time_list, _time_list[idx])
        end
    end

    dbserver.remote_call_game( "gmsvr.on_forbid_accounts_resp_from_db", _dpid, account_list, time_list, fail_player_id_list, _player_list, _is_forbid_machine )
    c_gm( "[db_gm](forbid_accounts_by_player_ids) dpid: %d, of players: %s, is_forbid_machine:%d", _dpid, player_id_str, _is_forbid_machine or 0 )
end


function query_account_info_by_account_id(_account_id, _callback_func_name, _param)
    local ret_code, account = db_login.try_update_account_cache(_account_id)
    dbserver.remote_call_game(_callback_func_name, _param, not account and {} or account)
end


-- 根据玩家名字正则表达式查找玩家id
function get_player_info_by_player_name_rex(_callback_func_name, _param)

    local player_list = {}

    local database = dbserver.g_db_character 

    -- local browser_client_id = _param.client_id_
    -- local player_name_rex = _param.player_name_rex_

    if database:c_stmt_format_query( SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL
                                   , SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL_PARAM_TYPE
                                   , _param.player_name_rex_
                                   , SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL_RESULT_TYPE
                                   ) < 0 then
        local err_msg = "[db_gm](get_player_info_by_player_name_rex) SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL error"
        -- gmclient.send_gm_cmd_result( browser_client_id, false, err_msg )
        dbserver.remote_call_game( _callback_func_name, _param, player_list)
        c_err( err_msg )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows < 1 then
--        local err_msg = sformat( "[db_gm](get_player_info_by_player_name_rex) no player named '%s'", _param.player_name_rex_ )
        -- gmclient.send_gm_cmd_result( browser_client_id, false, err_msg )
        dbserver.remote_call_game( _callback_func_name, _param, player_list)
--        c_err( err_msg )
        return
    end

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            
            local player = {}
            player.player_id_ = database:c_stmt_get( "player_id" )
            player.player_name_ = database:c_stmt_get( "player_name" )

            table.insert(player_list, player)
        else
            local err_msg = "[db_gm](get_player_info_by_player_name_rex) SELECT_PLAYER_ID_BY_PLAYER_NAME_REX_SQL fetch error "
            -- gmclient.send_gm_cmd_result( browser_client_id, false, err_msg )
            dbserver.remote_call_game( _callback_func_name, _param, player_list)
            c_err( err_msg )
            return
        end
    end

    dbserver.remote_call_game( _callback_func_name, _param, player_list)

    c_gm( "[db_gm](get_player_info_by_player_name_rex) player_list: %s", utils.serialize_table(player_list))
end


function query_player_batch(_player_id_list_str, _server_id)

    local player_list = {}


    local database = dbserver.g_db_character
    if database:c_stmt_format_query( sformat( SELECT_PLAYER_BATCH_SQL, _player_id_list_str ), SELECT_PLAYER_BATCH_SQL_PARAM_TYPE, SELECT_PLAYER_BATCH_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_arena](query_player_batch)  SELECT_PLAYER_BATCH_SQL failed" )
        return nil
    end

    local all_arena_player = {}
    local rows = database:c_stmt_row_num()

    -- local database = dbserver.g_db_character 
    -- if database:c_query( sformat( SELECT_PLAYER_BATCH_SQL, _player_id_list_str )  ) < 0 then
    --     c_err( "[db_login](query_player_batch) SELECT_PLAYER_BATCH_SQL error, player_id_list:%s", _player_id_list_str )
    --     return 
    -- end
    -- local rows = database:c_row_num()
    if rows <= 0 then
        c_err( "[db_gm](query_player_batch)not found:%s", _player_id_list_str )
        return player_list
    end

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local player = {}
            player.server_id = _server_id
            player.player_id, player.player_name, player.player_level, player.player_job
                = database:c_stmt_get("player_id",  "player_name", "level", "job_id")

            table.insert(player_list, player)
        else
            c_err( "[db_gm](query_player_batch) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end
    return player_list
end


function query_player_batch_new(_player_id_list_str, _server_id)

    local player_list = {}

    local database = dbserver.g_db_character
    if database:c_query(sformat( SELECT_PLAYER_BATCH_SQL, _player_id_list_str )) < 0 then
        c_err( "[db_gm](query_player_batch) SELECT_PLAYER_BATCH_SQL error")
        return
    end
    local rows = database:c_row_num()
    if rows <= 0 then
        c_err( "[db_gm](query_player_batch)not found:%s", _player_id_list_str )
        return player_list
    end
    for i = 1, rows, 1 do
        if database:c_fetch() == 0 then

            local r1, player_id = database:c_get_int32( "player_id" )
            local r2, player_name = database:c_get_str( "player_name", 64 )
            local r3, player_level = database:c_get_int32( "level")
            local r4, player_job = database:c_get_int32( "job_id" )

            if r1 < 0 or r2 < 0 or r3 < 0 or r4 < 0 then
                c_err("[db_gm](query_player_batch) c_get_xxx error")
                return {}
            end

            local player = {}
            player.server_id = _server_id
            player.player_id = player_id
            player.player_name = player_name
            player.player_level = player_level
            player.player_job = player_job

            table.insert(player_list, player)
        else
            c_err( "[db_gm](query_player_batch) c_fetch failed, index:%d", i)
            return {}
        end
    end
    return player_list
end
