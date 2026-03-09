module( "gm_cmd", package.seeall )

local sformat, sgmatch, tinsert, ostime = string.format, string.gmatch, table.insert, os.time

function reset_client_version_num( _dpid, _browser_client_id, _type, _version_num )
    local client_type = tonumber( _type )
    local version_num = tonumber( _version_num )

    return g_player_login_svr:c_reset_client_version( client_type, version_num )
end

function forbid_account( _account_id, _forbid_end_time )
    local db_succ, err_msg = db_service.update_forbid_info( _account_id, _forbid_end_time )
    if not db_succ then
        return false, err_msg
    end
    local now = ostime() 
    if _forbid_end_time > now then
        local remote_call_gs = game_login_server.remote_call_gs
        for dpid, server_id in pairs( game_login_server.g_dpid_2_server_id ) do
            remote_call_gs( dpid, "gm_cmd.kick_account_internal", _account_id )
        end
    end
    c_gm( "[gm_cmd](forbid_account) account_id: %d, forbid_end_time: %d", _account_id, _forbid_end_time )
    return true
end

function forbid_accounts( _dpid, _browser_client_id, _account_ids, _timestamps, _is_forbid_machine )
    local timestamp_tbl = {}
    for timestamp_str in sgmatch( _timestamps, "[^,]+" ) do
        tinsert( timestamp_tbl, tonumber( timestamp_str ))
    end

    local account_ids = {}
    local i = 1
    for account_id_str in sgmatch( _account_ids, "[^,]+" ) do
        local account_id = tonumber( account_id_str )
        if not account_id then
            return false, err( "[gm_cmd](forbid_accounts) account id is not number: '%s'", account_id_str )
        end
        account_ids[account_id] = timestamp_tbl[i]
        i = i + 1
    end

    local account_cnt = 0
    for account_id, forbid_end_time in pairs( account_ids ) do
        local ret, err_msg = forbid_account( account_id, forbid_end_time )
        if not ret then
            return false, err( err_msg )
        end

        if _is_forbid_machine and _is_forbid_machine == 1 then
            local machine_code = db_service.get_account_with_same_machine_code( account_id )
            if machine_code == "" then
                return false, err( sformat("[gm_cmd](forbid_accounts) cant get machine code by account_id:%d", account_id ) )
            else
                db_service.update_forbid_machine_info( machine_code, forbid_end_time )
            end
        end
        account_cnt = account_cnt + 1
    end

    local log_msg = sformat( "[gm_cmd](forbid_account) # of accounts: %d, forbid_end_time: %s", account_cnt, _timestamps )

    if _dpid == 0 then  -- 调用由game发起
        gmclient.send_gm_cmd_result( _browser_client_id, true, log_msg )
    else
        return true, log( log_msg )
    end
end

function switch_active_mode( _server_id, _is_open )
    local server_list = player_login_server.g_server_list
    local server_config = server_list[_server_id]

    if not server_config then
        c_err("cant find server_config by server_id:%d", _server_id)
        return
    end

    if _is_open == 1 then
        server_config.IsNeedActive = true
    else
        server_config.IsNeedActive = false
    end

    c_log("server_name:%s, server_id:%d, active status:%s", server_config.Name, server_config.GameServerId, server_config.IsNeedActive and "true" or "false")
end

function get_all_forbid_account(_dpid, _browser_client_id)
    local all_forbid_account = db_service.get_all_forbid_account()
    return all_forbid_account and true or false, all_forbid_account and cjson.encode(all_forbid_account) or "query db error"
end

