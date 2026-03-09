module( "bfclient", package.seeall )

local sformat, tinsert, mrandom, unpack, slen = string.format, table.insert, math.random, unpack, string.len
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

g_ar = g_ar or LAr:c_new()

g_bf_mng = g_bf_mng or {}
g_dpid2bfid_map = g_dpid2bfid_map or {}  -- key: bf_dpid; value: bf_id
g_is_zone_info_loaded_on_bf = g_is_zone_info_loaded_on_bf or {}  -- key: bf_id; value: true or false
g_bf_players = g_bf_players or {}

BF_PLAYER_STATE_GAME2BF     = 1
BF_PLAYER_STATE_BF          = 2
BF_PLAYER_STATE_BF2GAME     = 3

--rpc分包相关
g_split_rpc_data = g_split_rpc_data or {}
g_split_rpc_id = g_split_rpc_id or 0

function is_bf_registered( _bf_id )
    return g_bf_mng[_bf_id] ~= nil
end

function is_center_registered()
    return is_bf_registered( 0 )
end

function is_center_bf(_bf_id)
    return _bf_id == 0
end

function register_bf( _bf_id, _ip2client, _port2client )
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    local bf_info = {}
    bf_info.bf_id_ = _bf_id
    bf_info.ip2client_ = _ip2client
    bf_info.port2client_ = _port2client
    g_bf_mng[_bf_id] = bf_info
    g_dpid2bfid_map[bf_dpid] = _bf_id
    c_log( "[bfclient](register_bf) bf_id: %d, _ip2client: %s, _port2client: %d", _bf_id, _ip2client, _port2client )
    hall_mng.on_bf_register( _bf_id )
    egg_mng.on_bf_register( _bf_id )
end

function is_player_in_migrate(_player_id)
   return g_bf_players[ _player_id ]
end

function get_migrage_bf_id( _player_id )
    local p = g_bf_players[ _player_id ]
    return p and p.bf_id_
end

--------------------------------------------------------------------------------
--- bf获取的服务器列表和分区列表时的回调, 该回调只会被调用到一次
--- 1）如果gs重启时，连接上有bf时，有跨服逻辑需要跑
--- 2）跨服逻辑依赖服务器的分区和服务器列表信息
--  满足以下所有条件的逻辑加在这里,可以保证这时候调用bf时，bf上有所有的服务器信息
----------------------------------------------------------------------------
function on_bf_load_zone_info( _bf_id )
    g_is_zone_info_loaded_on_bf[_bf_id] = true
    rank_list_mng.on_bf_registered( _bf_id )
    gold_exchange_mng.on_bf_load_zone_info( _bf_id )
    shop_mng.on_bf_load_zone_info( _bf_id )
    wild_mng.check_open_first_wild()
    guild_daily_pvp.on_bf_load_zone_info( _bf_id )
    trade_mng.sync_item_list_to_bf()
    auction_mng.sync_item_list_to_bf()
    trade_statistic_mng.sync_trade_statistic_to_bf()
    event_team_ladder.on_bf_load_zone_info( _bf_id )
    event_team_champion.on_bf_load_zone_info( _bf_id )

    if is_center_bf( _bf_id ) then 
        honor_ladder_game.after_dbdata_ready_and_bf_connected()
        ares_witness_game.on_center_bf_connected()
        honor_witness_game.on_center_bf_connected()
    end 
end

function remove_bf_player( _player_id )
    local bf_player = g_bf_players[_player_id]
    if bf_player then
        local rejoin_timer_id = bf_player.rejoin_timer_id_
        if rejoin_timer_id then
            g_timermng:c_del_timer( rejoin_timer_id )
        end
        g_bf_players[_player_id] = nil
        c_log( "[bfclient](remove_bf_player) player_id: %d", _player_id )
    end

    local joining_player = gamesvr.g_joining_players[_player_id]
    if bf_player and joining_player then
        local bf_player_state = bf_player.state_
        if bf_player_state == BF_PLAYER_STATE_BF then
            -- 向数据库查询玩家的请求还未发出，因此执行离线操作
            for player_data_op_id, player_data_op in pairs( bf_player.player_data_ops_ ) do
                local func_for_offline_player = player_data_op.func_for_offline_player_
                if func_for_offline_player then
                    func_for_offline_player( _player_id, player_data_op.params_ )
                end
            end
            -- 玩家跨服的过程中，可能会一直连不上bf。这种情况下玩家可能会尝试重启游戏进程，重新连上game发PT_JOIN，
            -- 打算重新进游戏。此时如果bf那边的PLAYER_MIGRATE_TIMEOUT定时器还没超时，玩家就会因为角色还处在
            -- BF_PLAYER_STATE_BF状态而进不了游戏，客户端一直停留在选角界面转菊花。所以当PLAYER_MIGRATE_TIMEOUT
            -- 超时后，如果玩家还在joining列表中，就要把他送进游戏，不然客户端只要不关进程就会永远停留在选角界面
            -- 转菊花。
            gamesvr.do_join( _player_id, bf_player.account_id_ )
        else  -- 在此之前已经向数据库发出查询玩家的请求
            local player_data_ops = joining_player.player_data_ops_
            for player_data_op_id, player_data_op in pairs( bf_player.player_data_ops_ ) do
                tinsert( player_data_ops, player_data_op )
            end
            bf_player.player_data_ops_ = {}
        end
        c_log( "[bfclient](remove_bf_player) player joining, bf_player state: %d, player_id: %d", bf_player_state, _player_id )
    end
    return bf_player
end

function remove_bf_player_session( _player_id )
    local bf_player = remove_bf_player( _player_id )
    local account_id = bf_player.account_id_
    close_migrated_player_session_before_kick( account_id, _player_id )
    c_log( "[bfclient](remove_bf_player_session) account_id: %d, player_id: %d", account_id, _player_id )
end

function remove_player_data_op( _player_id, _player_data_op_id )
    local bf_player = g_bf_players[_player_id]
    if bf_player then
        bf_player.player_data_ops_[_player_data_op_id] = nil
    end
end

function send_player_to_bf( _player_id, _bf_id, _scene_id, _plane_id, _x, _z, _angle_y )
    local bf_info = g_bf_mng[_bf_id]
    if not bf_info then
        c_err( "[bfclient](send_player_to_bf) bf_id %d has not been registered in g_bf_mng yet", _bf_id )
        return
    end

    local player = player_mng.get_player_by_player_id( _player_id )
    if not player then
        c_err( "[bfclient](send_player_to_bf) player_mng contains no player of id %d, the player maybe offline", _player_id )
        return
    end

    if g_bf_players[_player_id] then
        c_err( "[bfclient](send_player_to_bf) player_id %d has already been migrating to bf", _player_id )
        return
    end

    local account_id = player.account_id_
    local bf_certify_code = mrandom( 2147483647 ) - 1  -- 2147483647 == math.pow(2, 31) - 1
    local migrate_time = os.time()

    -- add bf_player
    local bf_player =
    {
        player_id_ = _player_id,
        account_id_ = account_id,
        bf_id_ = _bf_id,
        state_ = BF_PLAYER_STATE_GAME2BF,
        bf_certify_code_ = bf_certify_code,
        scene_id_ = _scene_id,
        plane_id_ = _plane_id,
        x_ = _x,
        z_ = _z,
        angle_y_ = _angle_y,
        migrate_time_ = migrate_time,
        player_data_op_id_prev_ = 0,
        player_data_ops_ = {},
        level_ = player.level_, 
        head_icon_id_ = player.head_icon_id_,  
        total_gs_ = player.total_gs_, 
        player_name_ = player.player_name_, 
		avatar_main_fight_ = player.avatar_main_fight_,
		use_model_ = player.use_model_,
		job_id_ = player.job_id_,
    }
    g_bf_players[_player_id] = bf_player

    -- 先把bf_certify_code发给bf和客户端，等bf和客户端都确认后再踢客户端下线，让客户端连接bf。这是因为客户端连接上bf后
    -- 会马上向bf发注册验证包（PT_ASK_BF_REGISTER_DPID），此时bf需要持有bf_certify_code对客户端进行验证。
    remote_call_bf( _bf_id, "bfsvr.add_migrating_player", g_gamesvr:c_get_server_id(), _player_id, bf_certify_code, migrate_time )
    player:add_migrate_bf_begin( bf_info.ip2client_, bf_info.port2client_, bf_certify_code )

    if player:is_in_guild_scene() then
        guild_scene_mng.on_player_quit_battlefield( player, true, true, true ) 
    end
end

local function do_send_player_to_bf( _bf_player )
    if not _bf_player.added_to_bf_ then
        return
    end

    if not _bf_player.player_send_migrate_begin_succ_ then
        return
    end

    local player_id = _bf_player.player_id_

    local player = player_mng.get_player_by_player_id( player_id )
    if not player then
        remove_bf_player( player_id )
        c_err( "[bfclient](do_send_player_to_bf) player_mng contains no player of id %d, the player maybe offline", player_id )
        return
    end

    local bf_id = _bf_player.bf_id_

    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](do_send_player_to_bf) the dpid of bf_id %d is invalid", bf_id )
        return
    end

    -- do send
    c_log( "[bfclient](do_send_player_to_bf) do send player %d to bf %d", player_id, bf_id )

    local player_dpid = player.dpid_
    local player_dpid_info = gamesvr.g_dpid_mng[player_dpid]

    -- 更新玩家最后在主城的位置和朝向(必须在临时更改player.scene_id_之前)
    player:update_last_city_pos()

    -- 在serialize_to_peer之外，需要额外发送给bf的数据
    local extra_data_for_bf = player.extra_data_for_bf_
    extra_data_for_bf.plane_id_ = _bf_player.plane_id_
    extra_data_for_bf.dpid_info_ = player_dpid_info
    extra_data_for_bf.log_info_ = player.log_info_
    extra_data_for_bf.logclient_group_id_ = player.logclient_group_id_
    extra_data_for_bf.logclient_plat_form_ = player.logclient_plat_form_
    extra_data_for_bf.last_city_x_ = player.last_city_x_
    extra_data_for_bf.last_city_z_ = player.last_city_z_
    extra_data_for_bf.last_city_angle_y_ = player.last_city_angle_y_

    -- 备份帐号信息，玩家回原服时不会再从db获得帐号信息
    _bf_player.dpid_info_ = player_dpid_info

    local core = player.core_

    -- 备份相关属性
    local backup_scene_id = player.scene_id_
    local backup_x, backup_y, backup_z = core:c_getpos()
    local backup_angle_y = core:c_get_angle_y()

    -- 设置需要序列化给bf的属性
    player.scene_id_ = _bf_player.scene_id_
    core:c_setpos( _bf_player.x_, 0, _bf_player.z_ )
    core:c_set_angle_y( _bf_player.angle_y_ )

    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_G2B_SEND_PLAYER )
    player:serialize_to_peer( ar )
    ar:write_buffer( c_msgpack( extra_data_for_bf ) )
    ar:send_one_ar( g_bfclient, bf_dpid )

    -- 恢复备份的属性
    player.scene_id_ = backup_scene_id
    core:c_setpos( backup_x, backup_y, backup_z )
    core:c_set_angle_y( backup_angle_y )

    -- 向bf发送玩家数据的同时踢玩家下线，保证bf收到的数据和玩家下线时的数据一致。
    login_mng.before_player_migrate_bf( player.account_id_, player_id )
    player_mng.del_player( g_gamesvr:c_get_server_id(), player_id )
    g_gamesvr:c_kick_player( player_dpid )
end

function on_migrating_player_added( _player_id )
    local bf_player = g_bf_players[_player_id]
    if not bf_player then
        c_err( "[bfclient](on_migrating_player_added) player_id: %d, player is not in the process of migrating", _player_id )
        return
    end
    c_log( "[bfclient](on_migrating_player_added) player_id: %d", _player_id )
    bf_player.added_to_bf_ = true
    do_send_player_to_bf( bf_player )
end

function on_player_send_migrate_begin_succ( _dpid )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[bfclient](on_player_send_migrate_begin_succ) cannot get the player of dpid 0x%X", _dpid )
        return
    end

    local player_id = player.player_id_

    local bf_player = g_bf_players[player_id]
    if not bf_player then
        player:add_migrate_bf_fail()
        c_err( "[bfclient](on_player_send_migrate_begin_succ) player is not in the process of migrating, "
             .."player_id: %d, dpid: 0x%X", player_id, _dpid
             )
        return
    end

    c_log( "[bfclient](on_player_send_migrate_begin_succ) player_id: %d, dpid: 0x%X", player_id, _dpid )
    bf_player.player_send_migrate_begin_succ_ = true
    do_send_player_to_bf( bf_player )
end

function on_player_migrate_timeout( _player_id )
    remove_bf_player( _player_id )
    c_log( "[bfclient](on_player_migrate_timeout) player_id: %d", _player_id )
end

function on_bf_disconnect( _ar, _dpid, _size )
    local bf_id = g_dpid2bfid_map[_dpid]
    if not bf_id then
        c_err( "[bfclient](on_bf_disconnect) g_dpid2bfid_map has no entry of dpid 0x%X", _dpid )
        return
    end
    g_dpid2bfid_map[_dpid] = nil

    g_is_zone_info_loaded_on_bf[bf_id] = false

    local bf_info = g_bf_mng[bf_id]
    if not bf_info then
        c_err( "[bfclient](on_bf_disconnect) g_bf_mng has no entry of bf_id %d; dpid: 0x%X", bf_id, _dpid )
        return
    end

    hall_mng.on_bf_disconnect( bf_id )

    -- 清空该bf上所有玩家的跨服状态。
    local players_on_this_bf = {}
    local accounts_on_this_bf = {}

    for player_id, bf_player in pairs( g_bf_players ) do
        if bf_player.bf_id_ == bf_id then
            players_on_this_bf[ player_id ] = true
            accounts_on_this_bf[ bf_player.account_id_ ] = true
        end
    end

    for player_id in pairs( players_on_this_bf ) do
        remove_bf_player( player_id )
    end

    for account_id in pairs( accounts_on_this_bf ) do
        login_mng.migrated_player_certify( account_id, false )
    end

    c_log( "[bfclient](on_bf_disconnect) bf_id: %d, dpid: 0x%X", bf_id, _dpid )

    login_mng.on_bf_disconnect( players_on_this_bf )
    shop_mng.on_bf_disconnect( bf_id )
    gold_exchange_mng.on_bf_disconnect( bf_id )

    -- 战场玩家跨服信息
    tamriel_relic.tamriel_on_bf_disconnect( bf_id )
    five_pvp_match_gs.on_bf_disconnect( bf_id )
    event_weekend_pvp.on_bf_disconnect( bf_id )
    guild_daily_pvp.on_bf_disconnect( bf_id )
    auction_mng.on_bf_disconnect( bf_id )
    trade_mng.on_bf_disconnect( bf_id )
    event_team_ladder.on_bf_disconnect( bf_id )
    event_team_champion.on_bf_disconnect( bf_id )
    
    g_bf_mng[bf_id] = nil
end

--add by kent
--function on_bf_remote_call( _ar, _dpid, _size )
    --local func_name = _ar:read_string()
    --local arg_str   = _ar:read_buffer()

    --if g_debug then
        --c_trace( "[bfclient](on_bf_remote_call) execute remote call: %s", func_name )
    --end

    --local func = loadstring( sformat( "return _G.%s", func_name ) )()
    --if not func then
        --c_err( "[bfclient](on_bf_remote_call) failed to load function '%s'", func_name )
        --return
    --end

    --local args_table = c_msgunpack( arg_str )
    --func( unpack( args_table ) )
--end
function on_bf_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    if g_debug then
        c_trace( "[bfclient](on_bf_remote_call) execute remote call: %s", func_name )
    end

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then
        c_err( "[bfclient](on_bf_remote_call) failed to load function '%s'", func_name )
        return
    end

    if func_name == "hall_mng.on_center_get_upvote_data_for_client" then
        local arg_str   = _ar:read_string()
        local func_arg = loadstring(arg_str) 
        local args_table = func_arg()
        func( unpack( args_table ) )
    else
        local arg_str   = _ar:read_buffer()
        local args_table = c_msgunpack( arg_str )
        func( unpack( args_table ) )
    end
end
--

function on_bf_split_remote_call( _ar, _dpid, _size )
    local split_rpc_id = _ar:read_double()
    local sub_data = _ar:read_buffer()
    local bf_id = g_dpid2bfid_map[_dpid]
    if not bf_id then
        c_err( "[bfclient](on_bf_split_remote_call) no bf_id of dpid %d", _dpid )
        return
    end

    local bf_data_list = g_split_rpc_data[bf_id] or {}
    g_split_rpc_data[bf_id] = bf_data_list
    local data_list = bf_data_list[split_rpc_id] or {}
    bf_data_list[split_rpc_id] = data_list
    tinsert(data_list, sub_data)
end

function on_bf_split_remote_call_end( _ar, _dpid, _size )
    local split_rpc_id = _ar:read_double()
    local func_name = _ar:read_string()

    local bf_id = g_dpid2bfid_map[_dpid]
    if not bf_id then
        c_err( "[bfclient](on_bf_split_remote_call_end) no bf_id of dpid %d", _dpid )
        return
    end

    local bf_data_list = g_split_rpc_data[bf_id] or {}
    g_split_rpc_data[bf_id] = bf_data_list
    local data_list = bf_data_list[split_rpc_id] or {}
    bf_data_list[split_rpc_id] = nil

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then
        c_err( sformat( "[dbclient](on_db_remote_call)func %s not found", func_name ) )
        return
    end

    local data = table.concat(data_list, "")
    local args_table = c_msgunpack( data )
    func( unpack( args_table ) )
end

function on_bf_remote_call_to_bf( _ar, _dpid, _size )
    local bf_id = _ar:read_int()
    local func_name = _ar:read_string()
    local arg_str   = _ar:read_buffer()
    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_G2B_REMOTE_CALL )
    ar:write_string( func_name )
    ar:write_buffer( arg_str )

    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](on_bf_remote_call_to_bf) from bf dipid:0x%x,  failed to call function '%s' on bf_id %d: bf_dpid is invalid",  _dpid, tostring(func_name), bf_id )
        return
    end
    ar:send_one_ar( g_bfclient, bf_dpid )
end

function on_bf_recv_player_result( _ar, _dpid, _size )
    local player_id = _ar:read_ulong()
    local result_code = _ar:read_byte()

    local bf_id = g_dpid2bfid_map[_dpid]
    if not bf_id then
        c_err( "[bfclient](on_bf_recv_player_result) no bf_id of dpid %d", _dpid )
        return
    end

    local bf_player = g_bf_players[player_id]
    if not bf_player then
        c_err( "[bfclient](on_bf_recv_player_result) bf_id: %d, no bf_player of player_id %d", bf_id, player_id )
        return
    end

    if bf_player.bf_id_ ~= bf_id then
        c_err( "[bfclient](on_bf_recv_player_result) player_id: %d, bf_player.bf_id_: %d, bf_id: %d, not match", player_id, bf_player.bf_id_, bf_id )
        return
    end

    if result_code == 0 then
        c_log( "[bfclient](on_bf_recv_player_result) bf_id: %d, player_id: %d", bf_id, player_id )
        bf_player.state_ = BF_PLAYER_STATE_BF
    else
        c_err( "[bfclient](on_bf_recv_player_result) bf_id: %d, player_id: %d, bf recv error, result_code: %d", bf_id, player_id, result_code )
        remove_bf_player( player_id )
    end
end

function prepare_player_rejoin( _player_id, _account_id, _rejoin_certify_code )
    local bf_player = g_bf_players[_player_id]
    if not bf_player then  -- 假如game曾经宕机重启，那g_bf_players就会为空。
        c_err( "[bfclient](prepare_player_rejoin) no bf_player of player_id: %d", _player_id )
        return
    end
    local bf_id = bf_player.bf_id_
    bf_player.rejoin_certify_code_ = _rejoin_certify_code
    login_mng.before_player_migrate_game( _account_id )
    c_log( "[bfclient](prepare_player_rejoin) bf_id: %d, player_id: %d, rejoin begin", bf_id, _player_id )
    remote_call_bf( bf_id, "bfsvr.on_game_prepare_player_rejoin_done", g_gamesvr:c_get_server_id(), _player_id )
end

function can_bf_save_player( _player_id, _migrate_time )
    local bf_player = g_bf_players[_player_id]

    -- 找不到bf_player的可能原因：
    -- 1、game曾经宕机重启，g_bf_players被清空过。
    -- 2、rejoin定时器在bf存盘的包到达之前就超时，把玩家从g_bf_players表中删除了。
    if not bf_player then
        c_err( "[bfclient](can_bf_save_player) no bf_player of player_id: %d", _player_id )
        return false
    end

    if _migrate_time ~= bf_player.migrate_time_ then
        c_err( "[bfclient](can_bf_save_player) migrate time inconsistent. player_id: %d, expected migrate_time: %u, actual migrate_time: %u",
            _player_id, bf_player.migrate_time_, _migrate_time )
        return false
    end

    return true
end

function on_bf_save_player( _ar, _dpid, _size )
    local online = _ar:read_boolean()
    local player_id = _ar:read_ulong()
    local bf_player = g_bf_players[player_id]

    local log_param_online = 0
    if online then
        log_param_online = 1
    end
    c_log( "[bfclient](on_bf_save_player) player_id: %d, online: %d, bf_player_state: %d", player_id, log_param_online, bf_player.state_ )

    if not online then
        local bf_state = bf_player.state_
        if bf_state == BF_PLAYER_STATE_GAME2BF then
            c_err( "[bfclient](on_bf_save_player) impossible bf_state game2bf" )
        elseif bf_state == BF_PLAYER_STATE_BF then
            bf_player.state_ = BF_PLAYER_STATE_BF2GAME
        elseif bf_state == BF_PLAYER_STATE_BF2GAME then
            -- do nothing
        end

        -- 玩家可能又连上了game并且等待进入游戏（不管是正在按流程回原服，还是从bf上主动下线后又登录game），
        -- 因此发完最后一个存盘的包后要检查一下，让可能正等待的玩家进游戏。
        local account_id = bf_player.account_id_
        if login_mng.g_migrated_waiting_rejoin_map[ account_id ] then
            login_mng.migrated_player_certify( account_id, true )
        end

        for player_data_op_id, player_data_op in pairs( bf_player.player_data_ops_ ) do
            local func_for_offline_player = player_data_op.func_for_offline_player_
            if func_for_offline_player then
                func_for_offline_player( player_id, player_data_op.params_ )
            end
        end
        bf_player.player_data_ops_ = {}

        if gamesvr.g_joining_players[player_id] and bf_state == BF_PLAYER_STATE_BF then
            gamesvr.do_join( player_id, bf_player.account_id_ )
        else
            -- 防止玩家与bf断线后再也不进入game，造成bf_player永久驻留在内存中。
            bf_player.rejoin_timer_id_ = g_timermng:c_add_timer_second( 30, timers.REJOIN_TIMEOUT, player_id, 0, 0 )
        end
    end
end

function notify_bf_player_login_other_place( _bf_player )
    local player_id = _bf_player.player_id_
    local account_id = _bf_player.account_id_
    local bf_id = _bf_player.bf_id_
    remote_call_bf( bf_id, "bfsvr.on_player_login_other_place", g_gamesvr:c_get_server_id(), player_id, account_id )
    c_log( "[bfclient](notify_bf_player_login_other_place) player_id: %d, bf_id: %d", player_id, bf_id )
end

function migrated_player_already_disconnect( _account_id )
    login_mng.migrated_player_certify( _account_id, false )
end

function close_migrated_player_session_before_kick( _account_id, _player_id )
    login_mng.close_session_before_kick_player( _account_id, _player_id, true )
end

function on_migrated_player_disconnect( _account_id, _player_id )
    local bf_player = g_bf_players[ _player_id ]
    if bf_player and bf_player.state_ == BF_PLAYER_STATE_BF2GAME then
        login_mng.migrated_player_disconnect( _account_id, _player_id )
    end
    c_log( "[bfclient](on_migrated_player_disconnect) account_id: %d, player_id: %d", _account_id, _player_id )
end

function on_bf_five_pvp_match_succ(_ar, _dpid, _size)
    local pid = _ar:read_int()
    local camp = _ar:read_ubyte()

    local player = player_mng.get_player_by_player_id(pid)
    if player then
        player:add_five_pvp_match_start(camp)
    end
end

function on_bf_five_pvp_match_add(_ar, _dpid, _size)
    local pid = _ar:read_int()
    local add_pid = _ar:read_int()
    local add_sid = _ar:read_int()
    local name = _ar:read_string()
    local icon = _ar:read_int()
    local camp = _ar:read_ubyte()
    local competitive_halo_rate =  _ar:read_float()

    local player = player_mng.get_player_by_player_id(pid)
    if player then
        player:add_add_five_pvp_match(add_pid, add_sid, name, icon, camp, competitive_halo_rate)
    end
end

function on_bf_five_pvp_match_remove(_ar, _dpid, _size)
    local to_pid = _ar:read_int()
    local rm_pid = _ar:read_int()
    local rm_sid = _ar:read_int()

    local player = player_mng.get_player_by_player_id(to_pid)
    if player then
        player:add_rm_five_pvp_match(rm_pid, rm_sid)
    end
end

func_table =
{
    [msg.PT_DISCONNECT] = on_bf_disconnect,
    [msg.BF_TYPE_B2G_REMOTE_CALL] = on_bf_remote_call,
    [msg.BF_TYPE_B2G_SPLIT_REMOTE_CALL] = on_bf_split_remote_call,
    [msg.BF_TYPE_B2G_SPLIT_REMOTE_CALL_END] = on_bf_split_remote_call_end,
    [msg.BF_TYPE_B2B_REMOTE_CALL] = on_bf_remote_call_to_bf,
    [msg.BF_TYPE_B2G_RECV_PLAYER_RESULT] = on_bf_recv_player_result,
    [msg.BF_TYPE_B2G_SAVE_PLAYER] = on_bf_save_player,

    [msg.BF_TYPE_B2G_FIVE_PVP_MATCH_SUCC] = on_bf_five_pvp_match_succ,
    [msg.BF_TYPE_B2G_FIVE_PVP_MATCH_ADD] = on_bf_five_pvp_match_add,
    [msg.BF_TYPE_B2G_FIVE_PVP_MATCH_REMOVE] = on_bf_five_pvp_match_remove,
}

function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then
        (func_table[_packet_type])( _ar, _dpid, _size )
    else
        c_err( "[LUA_MSG_HANDLE](bfclient) unknown packet 0x%08x %08x", _packet_type, _dpid )
    end
end

function is_bf_connected( _bf_id )
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    return bf_dpid ~= share_const.DPID_UNKNOWN
end

function remote_call_bf( _bf_id, _func_name, ... )
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](remote_call_bf) failed to call function '%s' on bf_id %d: bf_dpid is invalid", _func_name, _bf_id )
        return
    end

    local ar = g_ar
    local arg = {...}
    ar:flush_before_send( msg.BF_TYPE_G2B_REMOTE_CALL )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_bfclient, bf_dpid )
end

function split_remote_call_bf(_bf_id, _func_name, ...)
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](remote_call_bf) failed to call function '%s' on bf_id %d: bf_dpid is invalid", _func_name, _bf_id )
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

        ar:flush_before_send( msg.BF_TYPE_G2B_SPLIT_REMOTE_CALL )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_bfclient, bf_dpid )
    end

    if total_len > count * unit_len then
        local begin_idx = 1 + count * unit_len
        local end_idx = total_len
        local sub_data = string.sub(data, begin_idx, end_idx)

        ar:flush_before_send( msg.BF_TYPE_G2B_SPLIT_REMOTE_CALL )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_bfclient, bf_dpid )
    end

    ar:flush_before_send( msg.BF_TYPE_G2B_SPLIT_REMOTE_CALL_END )
    ar:write_double( split_rpc_id )
    ar:write_string( _func_name )
    ar:send_one_ar( g_bfclient, bf_dpid )
end

function remote_call_center( _func_name, ... )
    remote_call_bf( 0, _func_name, ... )
end

function remote_call_gs_by_bf(_svr_id, _func_name, ...)
    local ar = g_ar
    local arg = {...}
    local succ, bf_id, dpid = g_bfclient:c_get_random_bf()
    if not succ then
        c_err("[bfclient](remote_call_gs_by_bf)no bf connected!")
        return
    end
    c_trace("[bfclient](remote_call_gs_by_bf) bfid:%d, func_name:%s", bf_id, _func_name )

    ar:flush_before_send( msg.BF_TYPE_G2G_BY_BF )
    ar:write_int(_svr_id)
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_bfclient, dpid)
end


function remote_call_gs_by_bf_ex(_svr_id, _func_name, ...)
    local ar = g_ar
    local arg = {...}
    local succ, bf_id, dpid = g_bfclient:c_get_bf_by_serverid()
    if not succ then
          c_err("[bfclient](remote_call_gs_by_bf_ex)no bf connected!")
          return
    end
    c_trace("[bfclient](remote_call_gs_by_bf_ex) bfid:%d, func_name:%s", bf_id, _func_name )

    ar:flush_before_send( msg.BF_TYPE_G2G_BY_BF )
    ar:write_int(_svr_id)
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_bfclient, dpid)
end

function remote_call_zone_by_bf(_svr_id, _exclude_svr_id, _func_name, ...)
    local ar = g_ar
    local arg = {...}
    local succ, bf_id, dpid = g_bfclient:c_get_random_bf()
    if not succ then
        c_err("[bfclient](remote_call_zone_by_bf)no bf connected!")
        return
    end
    c_trace("[bfclient](remote_call_zone_by_bf) bfid:%d, func_name:%s", bf_id, _func_name )

    ar:flush_before_send( msg.BF_TYPE_G2Z_BY_BF )
    ar:write_int( _svr_id )
    ar:write_int( _exclude_svr_id )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_bfclient, dpid)
end

function remote_call_all_bfs( _func_name, ... )
    local ar = g_ar
    local arg = {...}
    local dpids = g_bfclient:c_get_all_dpid()
    for k, v in pairs( dpids ) do
        ar:flush_before_send( msg.BF_TYPE_G2B_REMOTE_CALL )
        ar:write_string( _func_name )
        ar:write_buffer( c_msgpack( arg ) )
        ar:send_one_ar( g_bfclient, v )
    end
end

function remote_call_all_bfs_without_center( _func_name, ... )
    local ar = g_ar
    local arg = {...}
    local dpids = g_bfclient:c_get_all_bf_dpid()
    for k, v in pairs( dpids ) do
        ar:flush_before_send( msg.BF_TYPE_G2B_REMOTE_CALL )
        ar:write_string( _func_name )
        ar:write_buffer( c_msgpack( arg ) )
        ar:send_one_ar( g_bfclient, v )
    end
end

function to_bf_five_add_to_match_list(_bf_id, _param )
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](add_to_match_list) failed on bf_id %d: bf_dpid is invalid", _bf_id )
        return
    end

    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_G2B_FIVE_PVP_ADD_TO_MATCH_LIST)
    ar:write_buffer( c_msgpack( _param ) )
    ar:send_one_ar( g_bfclient, bf_dpid )
end

function to_bf_five_del_from_match_list(_bf_id, _param)
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](del_from_match_list) failed on bf_id %d: bf_dpid is invalid", _bf_id )
        return
    end

    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_G2B_FIVE_PVP_DEL_FROM_MATCH_LIST)
    ar:write_buffer( c_msgpack( _param) )
    ar:send_one_ar( g_bfclient, bf_dpid )
end

function to_bf_tamriel_add_to_match_list(_bf_id, _param )
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](add_to_match_list) failed on bf_id %d: bf_dpid is invalid", _bf_id )
        return
    end

    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_G2B_TAMRIEL_ADD_TO_MATCH_LIST)
    ar:write_buffer( c_msgpack( _param ) )
    ar:send_one_ar( g_bfclient, bf_dpid )
end

function to_bf_tamriel_del_from_match_list(_bf_id, _param)
    local bf_dpid = g_bfclient:c_get_dpid_by_bfid( _bf_id )
    if bf_dpid == share_const.DPID_UNKNOWN then
        c_err( "[bfclient](del_from_match_list) failed on bf_id %d: bf_dpid is invalid", _bf_id )
        return
    end

    local ar = g_ar
    ar:flush_before_send( msg.BF_TYPE_G2B_TAMRIEL_DEL_FROM_MATCH_LIST)

    ar:write_buffer( c_msgpack(_param) )
    ar:send_one_ar( g_bfclient, bf_dpid )
end

