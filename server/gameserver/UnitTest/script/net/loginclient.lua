module( "loginclient", package.seeall )

local sformat, unpack = string.format, unpack
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

g_ar = g_ar or LAr:c_new()
g_heartbeat_timer = g_heartbeat_timer or nil

CERTIFY_PLAYER_TIMEOUT = 1

LOGIN_REPLY_TIMEOUT = 10

function remote_call_timeout( _id, _p2 )
    if _id == CERTIFY_PLAYER_TIMEOUT then 
        c_err( "[loginclient](remote_call_timeout) _dpid:0x%X", _p2 )     
        gamesvr.send_certify_code_to_client( _p2, login_code.LOGIN_ACCOUNT_TIMEOUT )
    else   
        c_err( "[loginclient](remote_call_timeout) unknow remote_call_timeout timer id:%d", _id )     
    end
end

function verify_account( _account_id, _login_token, _dpid )
    local timer_id = g_timermng:c_add_timer_second( LOGIN_REPLY_TIMEOUT, timers.GL_REMOTE_CALL, CERTIFY_PLAYER_TIMEOUT, _dpid, 0)
    remote_call_ls("game_login_server.on_player_certify", _account_id, _login_token, _dpid, timer_id)
end

function send_join_to_login( dpid, _player_id, _account_id )
    remote_call_ls( "game_login_server.on_player_join", dpid, _player_id, _account_id )
end

function on_login_certify_result( _result_code, _account_id, _queue_free, _client_dpid, _timer_id )
    g_timermng:c_del_timer( _timer_id )
    
    if _result_code == login_code.LOGIN_CERTIFY_OK then
        login_mng.login( _client_dpid, _account_id, _queue_free )
    else    
        local ext_param = _queue_free  -- 当状态为封禁时，这个参数用来传封禁结束时间
        gamesvr.send_certify_code_to_client( _client_dpid, _result_code, nil, ext_param )
        c_err( "[loginclient](on_login_certify_result) certify failed account_id: %d, dpid: 0x%X with login code: %d", _account_id, _client_dpid, _result_code )
    end
end

function on_game_register_failed( _err_str )
    c_err("[loginclient](on_game_register_failed)exit as:%s", _err_str)
    c_post_disconnect_msg( g_loginclient, 0 )
    c_assert(false, _err_str) 
end

function on_login_player_join( _dpid, _player_id, _account_info )
    if not _account_info then
        c_err( "[loginclient](on_login_player_join) dpid: 0x%X, _player_id: %d account not found, check login for details", _dpid, _player_id )
        gamesvr.send_player_join_result_to_client( _dpid, login_code.PLAYER_JOIN_QUERY_LOGIN_TBL_ERR )
        return
    end
    local joining_player = gamesvr.g_joining_players[_player_id]
    if joining_player then
        joining_player.account_info_from_login_ = _account_info
        gamesvr.do_join_on_recv_from_peer( joining_player )
    else
        mail_box_t.add_return_diamond_mail( _player_id,
                                            _account_info.account_id_,
                                            _account_info.return_bind_diamond_,
                                            _account_info.return_non_bind_diamond_ )
        c_log( "[loginclient](on_login_player_join) dpid: 0x%X, player_id: %d, no such joining player found, the player may have disconnected", _dpid, _player_id )
    end
    c_log( "[loginclient](on_login_player_join) dpid: 0x%X, player_id: %d, account_id: %d, return_bind_diamond: %d, return_non_bind_diamond: %d"
         , _dpid, _player_id, _account_info.account_id_, _account_info.return_bind_diamond_, _account_info.return_non_bind_diamond_ )
end

----------------------------------------------------
--handle msg
function on_connected( _ar, _dpid, _size )
    local ar = g_ar
    ar:flush_before_send( msg.GL_TYPE_LOGIN )
    ar:write_uint( g_gamesvr:c_get_server_id() ) --server_id
    ar:write_string( g_gamesvr:c_get_server_ip() ) --server_ip
    ar:write_uint( g_gamesvr:c_get_server_port() ) --server_port
    ar:write_uint( g_playermng:c_get_count() ) --user_num
    ar:send_one_ar( g_loginclient, 0)

    g_heartbeat_timer = g_timermng:c_add_timer( 600, timers.LOGIN_CLIENT_HEARTBEAT, 0, 0, 600)
end

function on_disconnected( _ar, _dpid, _size )
    g_timermng:c_del_timer(g_heartbeat_timer)
    g_heartbeat_timer = nil
end

function on_heartbeat_timeout( data1,  data2 )
    local ar = g_ar
    ar:flush_before_send( msg.GL_TYPE_HEARTBEAT )
    ar:write_uint( g_playermng:c_get_count() ) --user_num
    ar:send_one_ar( g_loginclient, 0)
    return 1
end

function on_login_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args = _ar:read_buffer()
    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( "[loginclient](on_login_remote_call)func %s not found", func_name ) 
        return 
    end
    local args_table = c_msgunpack( args )
    func( unpack( args_table ) )
end

func_table =
{
    [msg.LG_TYPE_CONNECT] = on_connected,
    [msg.LG_TYPE_DISCONNECT] = on_disconnected,
    [msg.LG_TYPE_REMOTE_CALL] = on_login_remote_call,
}

function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        c_err( "[loginclient](message_hanlder) unknown packet 0x%08x %08x", _packet_type, _dpid )
    end  
end

function remote_call_ls( _func_name, ... )
    local ar = g_ar
    local arg = {...}
    ar:flush_before_send( msg.GL_TYPE_REMOTE_CALL )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_loginclient, 0 )
end
