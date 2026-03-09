module( "bfsvr", package.seeall )

local sformat, unpack, msg, ostime, tinsert, slen = string.format, unpack, msg, os.time, table.insert, string.len
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

HEART_BEAT_TIMEOUT = 30     -- second
CLOSE_CONNECTION_TIME_S = 2

g_ar = g_ar or LAr:c_new()

g_game_mng = g_game_mng or {}
g_game2dpid_map = g_game2dpid_map or {}
g_migrating_players = g_migrating_players or {}
g_dpid2migrating_players = g_dpid2migrating_players or {}
g_close_connection_timer_map = g_close_connection_timer_map or {}  -- key: string.format( "%d,%d", svr_id, player_id ) value: timer_id

g_center_bf_online_counter = g_center_bf_online_counter or {}

--rpc分包相关
g_split_rpc_data = g_split_rpc_data or {} 
g_split_rpc_id = g_split_rpc_id or 0

function operate_player_data( _server_id
                            , _player_id
                            , _player_data_op_id
                            , _func_name_for_online_player
                            , _params
                            ) 
    local func_for_online_player =  loadstring( sformat( "return _G.%s", _func_name_for_online_player ) )()
    if not func_for_online_player then
        c_err( "[bfsvr](operate_player_data) failed to load function '%s'", _func_name_for_online_player ) 
        return
    end

    local player = player_mng.get_player( _server_id, _player_id )
    if player then
        -- 先调用bfclient.remove_player_data_op删除game那边的数据，然后再调用func_for_online_player，
        -- 这样万一func_for_online_player在中途抛出异常，也不至于因为bfclient.remove_player_data_op没执行到，
        -- 而导致在玩家从bf下线后，game那边又将data_op执行了一遍，使得func_for_online_player已经添加了一部分
        -- 的数据又被重复添加一遍。
        remote_call_game( _server_id, "bfclient.remove_player_data_op", _player_id, _player_data_op_id )
        func_for_online_player( player, _params )
        c_log( "[bfsvr](operate_player_data) player online, server_id: %d, player_id: %d", _server_id, _player_id )
        return
    end

    local migrating_player = get_migrating_player( _server_id, _player_id )
    if migrating_player then
        migrating_player.player_data_ops_[_player_data_op_id] =
        {
            func_for_online_player_ = func_for_online_player,
            params_ = _params,
        }
        c_log( "[bfsvr](operate_player_data) player is migrating, server_id: %d, player_id: %d", _server_id, _player_id )
        return
    end

    c_log( "[bfsvr](operate_player_data) player may have returned to game, server_id: %d, player_id: %d", _server_id, _player_id )
end

function add_migrating_player( _svr_id, _player_id, _bf_certify_code, _migrate_time )
    remove_migrating_player( _svr_id, _player_id )  -- migrate的过程中，玩家还是可以关闭客户端重新进game重新跨服的
    local mng_one_gamesvr = g_migrating_players[_svr_id]
    if mng_one_gamesvr == nil then
        mng_one_gamesvr = {}
        g_migrating_players[_svr_id] = mng_one_gamesvr
    end
    local player =
    {
        server_id_ = _svr_id,
        bf_certify_code_ = _bf_certify_code,
        migrate_time_ = _migrate_time,
        migrate_timeout_id_ = g_timermng:c_add_timer_second( 30, timers.PLAYER_MIGRATE_TIMEOUT, _svr_id, _player_id, 0 ),  -- 30秒是飞飞的经验值
        player_data_ops_ = {},
    }
    mng_one_gamesvr[_player_id] = player
    c_log( "[bfsvr](add_migrating_player) server_id: %d, player_id: %d", _svr_id, _player_id )
    remote_call_game( _svr_id, "bfclient.on_migrating_player_added", _player_id )
end

function remove_migrating_player( _svr_id, _player_id )
    local mng_one_gamesvr = g_migrating_players[_svr_id]
    if mng_one_gamesvr then
        local player = mng_one_gamesvr[_player_id]
        if player then
            local migrate_timeout_id = player.migrate_timeout_id_
            if migrate_timeout_id then
                g_timermng:c_del_timer( migrate_timeout_id )
            end

            mng_one_gamesvr[_player_id] = nil
            
            local dpid = player.dpid_
            if dpid then
                g_dpid2migrating_players[dpid] = nil
            end
            return player
        end
    end
    return nil
end

function get_migrating_player( _svr_id, _player_id )
    local migrating_players_of_one_gamesvr = g_migrating_players[_svr_id]
    if not migrating_players_of_one_gamesvr then
        return nil
    end
    return migrating_players_of_one_gamesvr[_player_id]
end

function on_player_migrate_timeout( _svr_id, _player_id )
    local player = bfsvr.remove_migrating_player( _svr_id, _player_id )
    local player_dpid = player.dpid_
    if player_dpid then
        g_gamesvr:c_kick_player( player_dpid )
    end
    remote_call_game( _svr_id, "bfclient.on_player_migrate_timeout", _player_id )
    c_log( "[bfsvr](on_player_migrate_timeout) server_id: %d, player_id: %d", _svr_id, _player_id )
end

function on_game_register( _ar, _dpid, _size )
    local bf_id = _ar:read_int()
    local svr_id = _ar:read_int()
    
    if bf_id ~= g_bfsvr:c_get_bf_id() then
        c_err( "[bfsvr](on_game_register) register bf id: %d != bf_id: %d", bf_id, g_bfsvr:c_get_bf_id() )
        c_post_disconnect_msg( g_bfsvr, _dpid )
        return
    end

    local dpid = g_game2dpid_map[svr_id]
    if dpid then
        c_err( "[bfsvr](on_game_register) game server id %d repeat add", svr_id )
        c_post_disconnect_msg( g_bfsvr, _dpid )
        return
    end

    g_game_mng[_dpid] = 
    { 
        svr_id_ = svr_id, 
        heart_beat_ = ostime(),  
        timer_ = g_timermng:c_add_timer_second( HEART_BEAT_TIMEOUT, timers.GAME_HEARTBEAT, _dpid, 0, HEART_BEAT_TIMEOUT ),
    }
    g_game2dpid_map[svr_id] = _dpid
    c_log( "[bfsvr](on_game_register)game %d regsiter to bf %d succ, dpid: 0x%X", svr_id, bf_id, _dpid )
    remote_call_game( svr_id, "bfclient.register_bf", bf_id, g_gamesvr:c_get_server_ip_to_client(), g_gamesvr:c_get_server_port() )

    if bf_zone_mng.is_load_zone_info_finish() then
        remote_call_game( svr_id, "bfclient.on_bf_load_zone_info", bf_id)
    end
    
    team_mng.on_gameserver_register( svr_id )  -- GS和bf断开连接的时候使用bf的数据去修正或者恢复bf的team数据
end

function check_heart_beat( _dpid )
    local game_info = g_game_mng[_dpid]
    if game_info then
        if ostime() - game_info.heart_beat_ > HEART_BEAT_TIMEOUT then
            c_err( "[bfsvr](check_heart_beat)game %d timeout and be kicked", game_info.svr_id_ )
            c_post_disconnect_msg( g_bfsvr, _dpid )
        end
    end
end

function send_heart_beat_reply( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_HEART_BEAT_REPLY )
    ar:write_int( g_bfsvr:c_get_bf_id() )
    ar:send_one_ar( g_bfsvr, _dpid )
end

function on_game_heart_beat( _ar, _dpid, _size )
    local game_info = g_game_mng[_dpid]
    if game_info then
        game_info.heart_beat_ = ostime()
        send_heart_beat_reply( _dpid )
    end
end

function on_game_disconnect( _ar, _dpid, _size )
    local game_info = g_game_mng[_dpid]
    if game_info then
        local svr_id = game_info.svr_id_

        c_log( "[bfsvr](on_game_disconnect) remove game of svr_id %d", svr_id )

        -- 清空正在从这个game跨服过来的玩家
        local migrating_players_from_this_game = g_migrating_players[svr_id]
        if migrating_players_from_this_game then
            local migrating_player_ids = {}
            for player_id, player in pairs( migrating_players_from_this_game ) do
                tinsert( migrating_player_ids, player_id )
            end
            for i, player_id in ipairs( migrating_player_ids ) do
                remove_migrating_player( svr_id, player_id )
            end
        end

        -- 清空bf上来自该game的所有数据，类似db和game断开连接时，game自动停机，避免数据严重不一致。
        g_game_mng[_dpid] = nil
        g_timermng:c_del_timer( game_info.timer_ )
        g_game2dpid_map[svr_id] = nil
        player_mng.kick_all_players_of_server( svr_id )
        if bf_zone_mng.is_load_zone_info_finish() then
            shop_mng.clear_shops_of_game( svr_id )
            trade_mng.clear_server_trade_item( svr_id )
            auction_mng.clear_server_auction_item( svr_id )
            event_team_ladder.on_game_disconnect( svr_id )
        end
        gold_exchange_mng.clear_gold_exchanges_of_game( svr_id )
    end
end

function on_game_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local arg_str   = _ar:read_buffer()

    if g_debug then 
        c_trace( sformat( "[bfsvr](on_game_remote_call) execute remote call: %s", func_name ) )
    end  

    local func = loadstring( string.format( "return _G.%s", func_name ) )()
    if not func then return end

    local args_table = c_msgunpack( arg_str )
    func( unpack( args_table ) )
end

function on_game_split_remote_call( _ar, _dpid, _size )
    local split_rpc_id = _ar:read_double()
    local sub_data = _ar:read_buffer()
    local game_info = g_game_mng[_dpid]
    if not game_info then
        c_err( "[bfsvr](on_game_split_remote_call) no game_info of dpid %d", _dpid )
        return
    end

    local svr_id = game_info.svr_id_
    local game_data_list = g_split_rpc_data[svr_id] or {}
    g_split_rpc_data[svr_id] = game_data_list
    local data_list = game_data_list[split_rpc_id] or {}
    game_data_list[split_rpc_id] = data_list
    tinsert(data_list, sub_data)
end

function on_game_split_remote_call_end( _ar, _dpid, _size )
    local split_rpc_id = _ar:read_double()
    local func_name = _ar:read_string()

    local game_info = g_game_mng[_dpid]
    if not game_info then
        c_err( "[bfsvr](on_game_split_remote_call_end) no game_info of dpid %d", _dpid )
        return
    end

    local svr_id = game_info.svr_id_
    local game_data_list = g_split_rpc_data[svr_id] or {}
    g_split_rpc_data[svr_id] = game_data_list
    local data_list = game_data_list[split_rpc_id] or {}
    game_data_list[split_rpc_id] = nil 

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "[dbclient](on_db_remote_call)func %s not found", func_name ) )
        return 
    end

    local data = table.concat(data_list, "") 
    local args_table = c_msgunpack( data )
    func( unpack( args_table ) )
end

local function send_recv_player_result( _dpid, _player_id, _result_code )
    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_B2G_RECV_PLAYER_RESULT )
    ar:write_ulong( _player_id )
    ar:write_byte( _result_code )
    ar:send_one_ar( g_bfsvr, _dpid )
end

function on_game_send_player( _ar, _dpid, _size )
    local player = player_t.deserialize_from_peer( _ar )
    local extra_data_str = _ar:read_buffer()

    local extra_data = c_msgunpack( extra_data_str )
    for k, v in pairs( extra_data ) do
        player[k] = v
    end

    local player_id = player.player_id_

    local game_info = g_game_mng[_dpid]
    if not game_info then
        c_err( "[bfsvr](on_game_send_player) game_dpid: 0x%X, player_id: %d, no game info of this dpid", _dpid, player_id )
        send_recv_player_result( _dpid, player_id, 1 )
        return
    end

    local migrating_player = get_migrating_player( game_info.svr_id_, player_id )
    if not migrating_player then
        c_err( "[bfsvr](on_game_send_player) svr_id: %d, player_id: %d, no such migrating player", game_info.svr_id_, player_id )
        send_recv_player_result( _dpid, player_id, 2 )
        return
    end

    c_log( "[bfsvr](on_game_send_player) game_dpid: 0x%X, player_id: %d, goto_bf_event_id:%d recv success", _dpid, player_id, player.goto_bf_event_id_ or -1 )

    send_recv_player_result( _dpid, player_id, 0 )
    migrating_player.player_ = player
    player.migrate_time_ = migrating_player.migrate_time_
    gamesvr.do_join( migrating_player )
end

function send_player_back_to_game( _player )
    if _player.is_send_player_back_to_game_ then
        -- 加这个标志位,防止客户端发两次PT_QUIT_BATTLEFIELD
        return
    end

    _player.is_send_player_back_to_game_ = true

    local player_id = _player.player_id_
    local account_id = _player.account_id_
    local svr_id = _player.server_id_
    local rejoin_certify_code = math.random( 2147483647 ) - 1  -- 2147483647 == math.pow(2, 31) - 1
    remote_call_game( svr_id, "bfclient.prepare_player_rejoin", player_id, account_id, rejoin_certify_code )
    _player:add_rejoin_game_begin( rejoin_certify_code )
end

local function do_send_player_back_to_game( _player )
    if not _player.game_prepare_rejoin_done_ then
        return
    end
    if not _player.player_send_rejoin_begin_succ_ then
        return
    end
    local svr_id = _player.server_id_
    local player_id = _player.player_id_
    c_log( "[bfsvr](do_send_player_back_to_game) svr_id: %d, player_id: %d, do send player back to game", svr_id, player_id )
    player_mng.del_player( svr_id, player_id )  -- del_player中会给game发包保存玩家数据
    g_gamesvr:c_kick_player( _player.dpid_ )
end

function on_game_prepare_player_rejoin_done( _svr_id, _player_id )
    local player = player_mng.get_player( _svr_id, _player_id )
    if not player then
        c_err( "[bfsvr](on_game_prepare_player_rejoin_done) svr_id: %d, player_id: %d, player not found in player_mng", _svr_id, _player_id )
        return
    end
    c_log( "[bfsvr](on_game_prepare_player_rejoin_done) svr_id: %d, player_id: %d, game is ready", _svr_id, _player_id )
    player.game_prepare_rejoin_done_ = true
    do_send_player_back_to_game( player )
end

function on_player_send_rejoin_begin_succ( _dpid )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[bfsvr](on_player_send_rejoin_begin_succ) no player of dpid %d found in player_mng", _dpid )
        return
    end
    c_log( "[bfsvr](on_player_send_rejoin_begin_succ) dpid: 0x%X, svr_id: %d, player_id: %d, player is ready", _dpid, player.server_id_, player.player_id_ )
    player.player_send_rejoin_begin_succ_ = true
    do_send_player_back_to_game( player )
end

function send_save_player( _online, _player )
    _player:update_game_time_before_save( _online )
    local game_id = _player.server_id_
    local game_dpid = g_game2dpid_map[game_id]
    local ar = g_ar         
    ar:flush_before_send( msg.BF_TYPE_B2G_SAVE_PLAYER )
    ar:write_ulong( _player.migrate_time_ )
    ar:write_boolean( _online )
    _player:serialize_to_peer( ar, true )
    ar:send_one_ar( g_bfsvr, game_dpid )
end

function on_player_login_other_place( _svr_id, _player_id, _account_id )
    local player = player_mng.get_player( _svr_id, _player_id )
    if player then
        gamesvr.send_login_other_place_to_client( player.dpid_ )
        local timer_id = g_timermng:c_add_timer_second( CLOSE_CONNECTION_TIME_S, timers.CLOSE_CLIENT_CONNECTION, _svr_id, _player_id, 0 )
        g_close_connection_timer_map[ sformat( "%d,%d", _svr_id, _player_id ) ] = timer_id 
        c_log( "[bfsvr](on_player_login_other_place) svr_id: %d, player_id: %d ", _svr_id, _player_id ) 
    else
        remote_call_game( _svr_id, "bfclient.migrated_player_already_disconnect", _account_id ) 
    end
end

function close_connection_timer_callback( _svr_id, _player_id )
    local player = player_mng.get_player( _svr_id, _player_id )
    if player then
        player_mng.del_player( _svr_id, _player_id )
        g_gamesvr:c_kick_player( player.dpid_ )
        g_close_connection_timer_map[ sformat( "%d,%d", _svr_id, _player_id ) ] = nil
    end
    c_log( "[bfsvr](close_connection_timer_callback) svr_id: %d, player_id: %d", _svr_id, _player_id )
end

function on_player_join_game_with_bf_state( _svr_id, _player_id )
    local player = player_mng.get_player( _svr_id, _player_id )
    if player then
        player_mng.del_player( _svr_id, _player_id )
    end
    remove_migrating_player( _svr_id, _player_id )
    c_log( "[bfsvr](on_player_join_game_with_bf_state) svr_id: %d, player_id: %d", _svr_id, _player_id )
end

function remote_close_session_before_kick_player( _svr_id, _player_id, _account_id ) 
    remote_call_game( _svr_id, "bfclient.close_migrated_player_session_before_kick", _account_id, _player_id )
    c_log( "[bfsvr](remote_close_session_before_kick_player) svr_id: %d, player_id: %d, account_id: %d", _svr_id, _player_id, _account_id )
end

function on_game_remote_call_game_by_bf( _ar, _dpid, _size )
    local server_id = _ar:read_int()
    local func_name = _ar:read_string()
    local arg_str   = _ar:read_buffer()

    c_trace( "[bfsvr](on_game_remote_call_game_by_bf) execute remote call: %s server_id:%d", func_name , server_id) 

    local game_dpid = g_game2dpid_map[server_id]
    if not game_dpid then
        c_bt()
        c_err( sformat( "[bfsvr](remote_call_game)can't find game_dpid with game_id: %d", server_id ) )
        return
    end

    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_B2G_REMOTE_CALL )
    ar:write_string( func_name )
    ar:write_buffer( arg_str )
    ar:send_one_ar( g_bfsvr, game_dpid )
end

function on_game_remote_call_zone_by_bf( _ar, _dpid, _size )
    local server_id = _ar:read_int()
    local exclude_server_id = _ar:read_int()
    local func_name = _ar:read_string()
    local arg_str   = _ar:read_buffer()

    local server_list = bf_zone_mng.get_zone_server_list( server_id )
    c_trace( "[bfsvr](on_game_remote_call_zone_by_bf) execute remote call: %s server_id:%d, to server num:%d, exclude:%d", 
        func_name, server_id, #server_list, exclude_server_id or -1) 

    local ar = g_ar
    local msg_code = msg.BF_TYPE_B2G_REMOTE_CALL
    for _, a_server_id in ipairs( server_list ) do
        if a_server_id ~= exclude_server_id then
            local game_dpid = g_game2dpid_map[ a_server_id ]
            if not game_dpid then
                --c_err( sformat( "[bfsvr](remote_call_zone_by_bf)can't find game_dpid with game_id: %d", a_server_id ) )
            else
                ar:flush_before_send( msg_code )
                ar:write_string( func_name )
                ar:write_buffer( arg_str )
                ar:send_one_ar( g_bfsvr, game_dpid )
            end
        end
    end
end

function on_game_five_pvp_add_to_match_list(_ar, _dpid, _size)
    local arg_str = _ar:read_buffer()
    local player_param = c_msgunpack( arg_str )

    five_pvp.add_to_match_list(player_param)
end

function on_game_five_pvp_del_from_match_list(_ar, _dpid, _size)
    local arg_str = _ar:read_buffer()
    local player_param = c_msgunpack( arg_str )

    five_pvp.del_from_match_list(unpack(player_param))
end

function on_game_tamriel_add_to_match_list(_ar, _dpid, _size)
    local arg_str = _ar:read_buffer()
    local player_param = c_msgunpack( arg_str )

    tamriel_relic_center.add_player_to_match( player_param )
end

function on_game_tamriel_del_from_match_list(_ar, _dpid, _size)
    local arg_str = _ar:read_buffer()
    local player_param = c_msgunpack( arg_str )

    tamriel_relic_center.remove_player_from_match( unpack( player_param ) )
end

func_table =
{
    [msg.PT_DISCONNECT] = on_game_disconnect,
    [msg.BF_TYPE_REGISTER] = on_game_register,
    [msg.BF_TYPE_HEART_BEAT] = on_game_heart_beat,
    [msg.BF_TYPE_G2B_REMOTE_CALL] = on_game_remote_call,
    [msg.BF_TYPE_G2B_SPLIT_REMOTE_CALL] = on_game_split_remote_call,
    [msg.BF_TYPE_G2B_SPLIT_REMOTE_CALL_END] = on_game_split_remote_call_end,
    [msg.BF_TYPE_G2B_SEND_PLAYER] = on_game_send_player,
    [msg.BF_TYPE_G2G_BY_BF] = on_game_remote_call_game_by_bf,
    [msg.BF_TYPE_G2Z_BY_BF] = on_game_remote_call_zone_by_bf,


    [msg.BF_TYPE_G2B_FIVE_PVP_ADD_TO_MATCH_LIST] = on_game_five_pvp_add_to_match_list,
    [msg.BF_TYPE_G2B_FIVE_PVP_DEL_FROM_MATCH_LIST] = on_game_five_pvp_del_from_match_list,
    [msg.BF_TYPE_G2B_TAMRIEL_ADD_TO_MATCH_LIST] = on_game_tamriel_add_to_match_list,
    [msg.BF_TYPE_G2B_TAMRIEL_DEL_FROM_MATCH_LIST] = on_game_tamriel_del_from_match_list,
}

function is_center()
    return g_bfsvr:c_get_bf_id() == 0
end

function is_center_bf_id( _bf_id )
    return _bf_id == 0
end

function choose_idle_game()
    for k, v in pairs( g_game2dpid_map ) do
        return k, v
    end
    return nil, nil
end

function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        c_err( sformat( "[LUA_MSG_HANDLE](bfsvr) unknown packet 0x%08x %08x", _packet_type, _dpid) )
    end  
end

--add by kent
--function remote_call_game( _game_id, _func_name, ... )
    --local game_dpid = g_game2dpid_map[_game_id]
    --if not game_dpid then
        --c_err( sformat( "[bfsvr](remote_call_game)can't find game_dpid with game_id: %d", _game_id ) )
        --return
    --end
    --local ar = g_ar
    --local arg = {...}
    --ar:flush_before_send( msg.BF_TYPE_B2G_REMOTE_CALL )
    --ar:write_string( _func_name )
    --ar:write_buffer( c_msgpack( arg ) )
    --ar:send_one_ar( g_bfsvr, game_dpid )
--end

function remote_call_game( _game_id, _func_name, ... )
    local game_dpid = g_game2dpid_map[_game_id]
    if not game_dpid then
        c_err( sformat( "[bfsvr](remote_call_game)can't find game_dpid with game_id: %d", _game_id ) )
        return
    end
    local ar = g_ar
    local arg = {...}

    if _func_name == "hall_mng.on_center_get_upvote_data_for_client" then
        ar:flush_before_send( msg.BF_TYPE_B2G_REMOTE_CALL )
        ar:write_string( _func_name )
        ar:write_string( utils.serialize_table( arg ) )
        ar:send_one_ar( g_bfsvr, game_dpid )
    else 
        ar:flush_before_send( msg.BF_TYPE_B2G_REMOTE_CALL )
        ar:write_string( _func_name )
        ar:write_buffer( c_msgpack( arg ) )
        ar:send_one_ar( g_bfsvr, game_dpid )
    end
end
--

function split_remote_call_game(_game_id, _func_name, ...)
    local game_dpid = g_game2dpid_map[_game_id]
    if not game_dpid then
        c_err( sformat( "[bfsvr](split_remote_call_game)can't find game_dpid with game_id: %d", _game_id ) )
        return
    end

    local ar = g_ar
    local arg = {...}    
    local data = c_msgpack( arg )
    local split_rpc_id = g_split_rpc_id 
    g_split_rpc_id = g_split_rpc_id + 1
     
    local total_len = slen(data) 
    local unit_len = utils.SPLIT_PACKET_LEN
    local count = math.floor(total_len / unit_len )
    for i=1, count do 
        local begin_idx = 1 + (i-1 ) * unit_len
        local end_idx = i * unit_len
        local sub_data = string.sub(data, begin_idx, end_idx)  

        ar:flush_before_send( msg.BF_TYPE_B2G_SPLIT_REMOTE_CALL )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_bfsvr, game_dpid )
    end

    if total_len > count * unit_len then
        local begin_idx = 1 + count * unit_len
        local end_idx = total_len 
        local sub_data = string.sub(data, begin_idx, end_idx)  

        ar:flush_before_send( msg.BF_TYPE_B2G_SPLIT_REMOTE_CALL )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_bfsvr, game_dpid )
    end

    ar:flush_before_send( msg.BF_TYPE_B2G_SPLIT_REMOTE_CALL_END )
    ar:write_double( split_rpc_id )
    ar:write_string( _func_name )
    ar:send_one_ar( g_bfsvr, game_dpid )
end

function bf_to_game_five_match_succ( _game_id, _pid, _camp)
    local game_dpid = g_game2dpid_map[_game_id]
    if not game_dpid then
        c_err( sformat( "[bfsvr](fi)can't find game_dpid with game_id: %d", _game_id ) )
        return
    end
    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_B2G_FIVE_PVP_MATCH_SUCC)
    ar:write_int( _pid )
    ar:write_ubyte( _camp )
    ar:send_one_ar( g_bfsvr, game_dpid )
end

function bf_to_game_five_match_remove( _game_id, _tpid, _pid, _sid)
    local game_dpid = g_game2dpid_map[_game_id]
    if not game_dpid then
        c_err( sformat( "[bfsvr](five_match_remove)can't find game_dpid with game_id: %d", _game_id ) )
        return
    end
    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_B2G_FIVE_PVP_MATCH_REMOVE )
    ar:write_int( _tpid )
    ar:write_int( _pid )
    ar:write_int( _sid )
    ar:send_one_ar( g_bfsvr, game_dpid )
end

function bf_to_game_five_match_add( _game_id, _pid, _add_pid, _sid, _name, _icon, _camp, _competitive_halo_rate)
    local game_dpid = g_game2dpid_map[_game_id]
    if not game_dpid then
        c_err( sformat( "[bfsvr](five_match_add)can't find game_dpid with game_id: %d", _game_id ) )
        return
    end
    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_B2G_FIVE_PVP_MATCH_ADD)
    ar:write_int( _pid )
    ar:write_int( _add_pid )
    ar:write_int( _sid )
    ar:write_string( _name )
    ar:write_int( _icon )
    ar:write_ubyte( _camp )
    ar:write_float( _competitive_halo_rate )
    ar:send_one_ar( g_bfsvr, game_dpid )
end

function remote_call_all_games( _func_name, ... )
    local remote_call_game = remote_call_game
    for k, v in pairs( g_game2dpid_map ) do
        remote_call_game( k, _func_name, ... )
    end   
end

function remote_call_bf( _bf_id, _func_name, ... )
    local ar = g_ar
    local arg = {...}
    local game_id, game_dpid = choose_idle_game()
    if game_id then
        ar:flush_before_send( msg.BF_TYPE_B2B_REMOTE_CALL )
        ar:write_int( _bf_id )
        ar:write_string( _func_name )
        ar:write_buffer( c_msgpack( arg ) )
        ar:send_one_ar( g_bfsvr, game_dpid )
    else
        c_err( "[bfsvr](remote_call_bf)not idle game for remote_call_bf" )
    end
end

function remote_call_center( _func_name, ... )
    remote_call_bf( 0, _func_name, ... )
end

function remote_call_all_bfs( _func_name, ... )
    local bf_num = g_bfsvr:c_get_bf_num()
    for i = 0, bf_num-1, 1 do
        remote_call_bf( i, _func_name, ... )
    end
end

function remote_call_all_bfs_without_self( _func_name, ... )
    local bf_num = g_bfsvr:c_get_bf_num()
    local self_bf_id = g_bfsvr:c_get_bf_id()
    for i = 0, bf_num-1, 1 do
        if i ~= self_bf_id then
            remote_call_bf( i, _func_name, ... )
        end
    end
end

function remote_call_all_bfs_without_center( _func_name, ... )
    local bf_num = g_bfsvr:c_get_bf_num()
    for i = 1, bf_num-1, 1 do
        remote_call_bf( i, _func_name, ... )
    end
end

function remote_call_zone(_server_id, _func_name, ...)
    local server_list = bf_zone_mng.get_zone_server_list( _server_id )
    c_trace( "[bfsvr](remote_call_zone) execute remote call: %s server_id:%d, to server num:%d", 
        _func_name, _server_id, #server_list) 

    local ar = g_ar
    local arg = {...}
    local msg_code = msg.BF_TYPE_B2G_REMOTE_CALL
    for _, a_server_id in ipairs( server_list ) do
        local game_dpid = g_game2dpid_map[ a_server_id ]
        if not game_dpid then
            c_err( sformat( "[bfsvr](remote_call_zone)can't find game_dpid with game_id: %d", a_server_id ) )
        else
            ar:flush_before_send( msg_code )
            ar:write_string( _func_name )
            ar:write_buffer( c_msgpack(arg) )
            ar:send_one_ar( g_bfsvr, game_dpid )
        end
    end
end

function remote_call_zone_by_zone_id( _zone_id, _func_name, ... )
    local ar = g_ar
    local arg = {...}
    local msg_code = msg.BF_TYPE_B2G_REMOTE_CALL
    
    local server_list = bf_zone_mng.get_zone_server_list_by_zone_id( _zone_id )
    for _, server_id in ipairs( server_list ) do
        local game_dpid = g_game2dpid_map[ server_id ]
        if game_dpid then
            ar:flush_before_send( msg_code )
            ar:write_string( _func_name )
            ar:write_buffer( c_msgpack(arg) )
            ar:send_one_ar( g_bfsvr, game_dpid )
        else
            c_err( "[bfsvr](remote_call_zone_by_zone_id)can't find game_dpid by game_id: %d", server_id )
        end
    end
end

function on_game_server_report_online_count( _server_id, _count )
    g_center_bf_online_counter [ _server_id ] = _count
end 

function on_bfserver_get_zone_online_count( _bf_id, _zone_id, call_back_func, ... )
    local get_zone_by_server_id = bf_zone_mng.get_zone_by_server_id

    local total_online_count = 0
    for server_id, online_count in pairs( g_center_bf_online_counter ) do 
        local zone_id = get_zone_by_server_id( server_id ) 
        if _zone_id == zone_id then 
            total_online_count = total_online_count + online_count
        end 
    end 
    bfsvr.remote_call_bf( _bf_id, call_back_func, total_online_count, ... )
end 

function is_game_registered( _game_id )
    local dpid = g_game2dpid_map[_game_id]
    return dpid ~= nil
end

