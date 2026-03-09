module( "game_login_server", package.seeall )

--------------------------------------------------
-- global var
--------------------------------------------------
local sformat, unpack = string.format, unpack
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

g_glar = g_glar or LAr:c_new()

g_gameserver_status = g_gameserver_status or {} -- [server_id] = player_num
g_dpid_2_server_id = g_dpid_2_server_id or {}

--------------------------------------------------
-- game login server functions
--------------------------------------------------

function on_gameserver_login( _ar, _dpid, _size )
    local game_server_id = _ar:read_uint()
    local game_server_ip = _ar:read_string()
    local game_server_port = _ar:read_uint()
    local server_user_num = _ar:read_uint()

    local server_list = player_login_server.g_server_list
    if not server_list then
        c_err( "[game_login_server](on_gameserver_login) server list not initialized! server id:%d, ip:%s, port:%d", game_server_id, game_server_ip, game_server_port )
        c_post_disconnect_msg( g_game_login_svr, _dpid )
        return
    end

    local server_config = server_list[game_server_id]
    if not server_config then
        c_err( "[game_login_server](on_gameserver_login) unknown gameserver! server id:%d, ip:%s, port:%d", game_server_id, game_server_ip, game_server_port )
        c_post_disconnect_msg( g_game_login_svr, _dpid )
        return
    end

    local server_cfg_ip = server_config["Ip"]
    local server_cfg_port = server_config["Port"]
    if game_server_port ~= server_cfg_port then 
        local err_str = sformat("WRONG IP/PORT CONFIG beteen game and server list, game listen[%s:%d], server list cfg[%s:%d]", 
            game_server_ip, game_server_port, server_cfg_ip, server_cfg_port)
        remote_call_gs(_dpid, "loginclient.on_game_register_failed", err_str)
        c_err("[game_login_server](on_gameserver_login) gameserver ip! server id:%d, game listen[%s:%d], server list cfg[%s:%d]", 
            game_server_id, game_server_ip, game_server_port, server_cfg_ip, server_cfg_port )
        return
    end

    g_gameserver_status[ game_server_id ]  = server_user_num
    g_dpid_2_server_id[ _dpid ] = game_server_id

    c_log( "[game_login_server](on_gameserver_login) gameserver login: id:%d, ip:%s, port:%d, server_user_num:%d", game_server_id, game_server_ip, game_server_port, server_user_num )
end

function on_gameserver_logout( _ar, _dpid, _size )
    local server_id = g_dpid_2_server_id[ _dpid ]
    if server_id then
        g_gameserver_status[ server_id ] = nil
        g_dpid_2_server_id[ _dpid ] = nil
    end
end

function on_recv_heartbeat( _ar, _dpid, _size )
    local server_user_num = _ar:read_uint()
    local server_id = g_dpid_2_server_id[ _dpid ]
    if server_id then
        c_trace( "[game_login_server](on_recv_heartbeat) server[%d:%x] report server_user_num:%d", server_id, _dpid, server_user_num )
        g_gameserver_status[ server_id ] = server_user_num
    else
        c_err( "[game_login_server](on_recv_heartbeat) gameserver heartbeat without login" )
        c_post_disconnect_msg( g_game_login_svr, _dpid )
    end
end

function on_game_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args_str = _ar:read_buffer()
    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( "[game_login_server](on_game_remote_call)func %s not found", func_name )
        return 
    end
    local args_table = c_msgunpack( args_str )
    func( _dpid, unpack( args_table ) )
end

function on_player_certify( _dpid, _account_id, _login_token, _client_dpid, _timer_id )
    local result_code, queue_free = player_login_server.verify_account_when_enter_game( _account_id, _login_token )
    if not queue_free then
        queue_free = 0
    end
    remote_call_gs( _dpid, "loginclient.on_login_certify_result", result_code, _account_id, queue_free, _client_dpid, _timer_id )
end

function on_gm_query_player_info( _dpid, _browser_client_id, _player )
    local succ, account_info = db_service.get_account_info( _player.account_id_ )
    if not succ then
        local err_msg = sformat( "[game_login_server](on_gm_query_player_info) get channel info failed, account_id: %d", _player.account_id_ )
        gmclient.send_gm_cmd_result( _browser_client_id, false, err_msg )
        c_err( err_msg )
        return
    end
    _player.channel_account_ = account_info.channel_account_
    _player.account_info_ = account_info
    remote_call_gs( _dpid, "gm_cmd.on_query_player_info_result", _browser_client_id, _player )
end

function on_gmsvr_query_player_info( _dpid, _gm_dpid, _player )
    local succ, account_info = db_service.get_account_info( _player.account_id_ )
    if not succ then
        remote_call_gs( _dpid, "gmsvr.on_query_player_info_callback", _gm_dpid, nil )
        c_err( "[game_login_server](on_gmsvr_query_player_info) get channel info failed, account_id: %d", _player.account_id_ )
        return
    end

    _player.channel_account_ = account_info.channel_account_
    _player.account_info_ = account_info
    remote_call_gs( _dpid, "gmsvr.on_query_player_info_callback", _gm_dpid, _player )
end

function on_player_join( _game_dpid, _player_dpid, _player_id, _account_id )
    local game_server_id = g_dpid_2_server_id[ _game_dpid ]
    if not game_server_id then
        c_err( "[game_login_server](on_player_join) unknown game dpid: 0x%X", _game_dpid )
        return
    end

    local succ, account_info = db_service.get_account_info( _account_id )
    if not succ then
        c_err( "[game_login_server](on_player_join) get account info failed, game_server_id: %d, account_id: %d", game_server_id, _account_id )
        remote_call_gs( _game_dpid, "loginclient.on_login_player_join", _player_dpid, _player_id )
        return
    end

    local is_prev_test_user = account_info.is_prev_test_user_
    local return_bind_diamond = account_info.return_bind_diamond_
    local return_non_bind_diamond = account_info.return_non_bind_diamond_

    if return_bind_diamond > 0 or return_non_bind_diamond > 0 then
        if not db_service.clear_return_diamond( _account_id, game_server_id, _player_id ) then
            remote_call_gs( _game_dpid, "loginclient.on_login_player_join", _player_dpid, _player_id )
            return
        end
    end

    remote_call_gs( _game_dpid, "loginclient.on_login_player_join", _player_dpid, _player_id, account_info )

    c_log( "[game_login_server](on_player_join) game_server_id: %d, "
         .."_player_dpid: 0x%X, player_id: %d, account_id: %d, is_prev_test_user: %s, return_bind_diamond: %d, return_non_bind_diamond: %d"
         , game_server_id, _player_dpid, _player_id, _account_id, is_prev_test_user and "true" or "false", return_bind_diamond, return_non_bind_diamond )
end

function forbid_accounts( _game_dpid, _browser_client_id, _account_ids, _timestamps, _is_forbid_machine )
    gm_cmd.forbid_accounts( 0, _browser_client_id, _account_ids, _timestamps, _is_forbid_machine )
end

function gmsvr_forbid_account( _game_dpid, _gm_dpid, _account_list, _time_list)
    local count = #_account_list
    local fail_account_list = {}
    for i=1, count do 
        local account_id = _account_list[i]
        local forbid_end_time = _time_list[i]
        local ret = gm_cmd.forbid_account( account_id, forbid_end_time )

        if not ret then
            table.insert(fail_account_list, account_id)
        end
    end

    remote_call_gs( _game_dpid, "gmsvr.on_forbid_account_callback", _gm_dpid, fail_account_list )
end

function gmsvr_forbid_account_by_player_ids( _game_dpid, _gm_dpid, _account_list, _time_list, _player_list, _fail_player_list, _is_forbid_machine)
    local count = #_account_list
    for i=1, count do 
        local account_id = _account_list[i]
        local forbid_end_time = _time_list[i]
        local ret = gm_cmd.forbid_account( account_id, forbid_end_time )
        if not ret then
            table.insert(_fail_player_list, _player_list[i])
        end

        if _is_forbid_machine and _is_forbid_machine == 1 then
            local machine_code = db_service.get_account_with_same_machine_code( account_id )
            if machine_code == "" then
                c_gm( "[game_login_server](gmsvr_forbid_account_by_player_ids) cant get machine code by account_id:%d", account_id )
            else
                db_service.update_forbid_machine_info( machine_code, forbid_end_time )
            end
        end
    end

    remote_call_gs( _game_dpid, "gmsvr.on_forbid_account_callback", _gm_dpid, _fail_player_list )
end

function forbid_account_by_machine_code( _game_dpid, _gm_dpid, _machine_code_list, _time_list, _callback  )
    local fail_list = {}
    for i, machine_code in ipairs( _machine_code_list ) do
        local forbid_end_time = _time_list[i]
        db_service.update_forbid_machine_info( machine_code, forbid_end_time )
        local account_id_list = db_service.get_account_info_by_machine_code( machine_code )
        if account_id_list and #account_id_list > 0 then
            for _, account_id in ipairs( account_id_list ) do
                c_gm( "[game_login_server](forbid_account_by_machine_code) begin forbid account:%s, by machine_code:%s", account_id, machine_code )
                local ret = gm_cmd.forbid_account( account_id, forbid_end_time )
                local result = "success"
                if not ret then
                    table.insert( fail_list, sformat("account_id:%s, machine_code:%s", account_id, machine_code))
                    result = "faild"
                end
                c_gm( "[game_login_server](forbid_account_by_machine_code) end forbid account:%s, by machine_code: %s, resutl:%s", account_id, machine_code, result )
            end
        else
            table.insert( fail_list, sformat("no account found by machine_code:%s", machine_code) )
            c_err("[game_login_server](forbid_account_by_machine_code) find account faild by machine_code: %s", machine_code)
        end
    end

    remote_call_gs( _game_dpid, _callback, _gm_dpid, fail_list )
end

func_table =
{
    [msg.GL_TYPE_HEARTBEAT]                     = on_recv_heartbeat,
    [msg.GL_TYPE_LOGIN]                         = on_gameserver_login,
    [msg.GL_TYPE_LOGOUT]                        = on_gameserver_logout,
    [msg.GL_TYPE_REMOTE_CALL]                   = on_game_remote_call,
}

--------------------------------------------------
-- callback functions for c/c++
--------------------------------------------------
function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        c_err( "[game_login_server](message_handler) unknown packet 0x%08x %08x", _packet_type, _dpid )
    end  
end

function remote_call_gs( _dpid, _func_name, ... )
    local ar = g_glar 
    local arg = {...}
    ar:flush_before_send( msg.LG_TYPE_REMOTE_CALL )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_game_login_svr, _dpid )
end
