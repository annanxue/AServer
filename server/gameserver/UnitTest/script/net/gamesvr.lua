module( "gamesvr", package.seeall )

MINI_SERVER_ID = 0

local sformat, sgsub, ser_table, unpack, msg, msqrt, mceil, mfloor = string.format, string.gsub, utils.serialize_table, unpack, msg, math.sqrt, math.ceil, math.floor
local c_msgpack, c_msgunpack, ttype, tinsert, tconcat, tisempty = cmsgpack.pack, cmsgpack.unpack, type, table.insert, table.concat, utils.table.is_empty
local tostring = tostring
local LOG_HEAD = "[gamesvr]"

g_ar = g_ar or LAr:c_new()
g_account_name_2_dpid = g_account_name_2_dpid or {}
g_joining_players = g_joining_players or {}  -- 正在等待进入游戏的玩家
g_dpid2joining_players = g_dpid2joining_players or {}
g_trace_packet = g_trace_packet or false

function operate_player_data( _player_id, _func_name_for_online_player, _func_name_for_offline_player, _params )
    local func_for_online_player =  loadstring( sformat( "return _G.%s", _func_name_for_online_player ) )()
    if not func_for_online_player then
        c_err( "[gamesvr](operate_player_data) failed to load function '%s'", _func_name_for_online_player ) 
        return
    end

    local func_for_offline_player = _func_name_for_offline_player and
                                    loadstring( sformat( "return _G.%s", _func_name_for_offline_player ) )()
    if _func_name_for_offline_player and not func_for_offline_player then
        c_err( "[gamesvr](operate_player_data) failed to load function '%s'", _func_name_for_offline_player ) 
        return
    end

    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        func_for_online_player( player, _params )
        c_log( "[gamesvr](operate_player_data) player online, player_id: %d", _player_id )
        return
    end

    -- 玩家在bf上或正在跨服或正在从bf回game的过程中
    local bf_player = bfclient.g_bf_players[_player_id]
    if bf_player then
        local player_data_op_id = bf_player.player_data_op_id_prev_ + 1
        bf_player.player_data_op_id_prev_ = player_data_op_id
        local player_data_op =
        {
            func_for_online_player_ = func_for_online_player,
            func_for_offline_player_ = func_for_offline_player,
            params_ = _params,
        }
        bf_player.player_data_ops_[player_data_op_id] = player_data_op
        bfclient.remote_call_bf( bf_player.bf_id_
                               , "bfsvr.operate_player_data"
                               , g_gamesvr:c_get_server_id()
                               , _player_id
                               , player_data_op_id
                               , _func_name_for_online_player
                               , _params
                               ) 
        c_log( "[gamesvr](operate_player_data) player is on bf %d, player_id: %d, current player_data_op_id: %d", bf_player.bf_id_, _player_id, player_data_op_id )
        return
    end

    -- 玩家正从选人界面进游戏
    local joining_player = g_joining_players[_player_id]
    if joining_player then
        local player_data_op =
        {
            func_for_online_player_ = func_for_online_player,
            func_for_offline_player_ = func_for_offline_player,
            params_ = _params,
        }
        tinsert( joining_player.player_data_ops_, player_data_op )
        c_log( "[gamesvr](operate_player_data) player is joining, player_id: %d", _player_id )
        return
    end

    -- 玩家已下线
    if func_for_offline_player then
        func_for_offline_player( _player_id, _params )
    end
    c_log( "[gamesvr](operate_player_data) player is offline, player_id: %d", _player_id )
end

function game_server_remote_call_mini_server( _player , _func_name , ... )
    local ar = g_ar
    local arg = {...}
    ar:flush_before_send( msg.SM_TYPE_RPC )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_gamesvr, _player.dpid_ )
end 

function mini_server_remote_call_game_server( _dpid , _func_name , ... )
    local ar = g_ar
    local arg = {...}
    ar:flush_before_send( msg.MS_TYPE_RPC )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_gamesvr, _dpid )
end 

function on_client_certify( _ar, _dpid, _size )
	local account_id = _ar:read_double()
    local login_token = _ar:read_int()
    login_mng.certify( _dpid, account_id, login_token )
end

function on_get_random_name( _ar, _dpid, _size )
    local gender = _ar:read_ubyte()
    dbclient.send_get_random_name_to_db( _dpid, gender )
end

function on_unlock_random_name( _ar, _dpid, _size )
    dbclient.send_unlock_name_to_db( _dpid )
end

function on_create_player( _ar, _dpid, _size )
    local account_id = _ar:read_double()
    local job_id = _ar:read_ulong()
    local player_name = _ar:read_string()
    local slot = _ar:read_ulong()
    if job_id < JOBS.JOB_ID_BEGIN or job_id > JOBS.JOB_ID_END then
        c_err( "[gamesvr](on_create_player) job id out of range, job_id: %d, account_id: %d, dpid: 0x%X", job_id, account_id, _dpid )
        return
    end

    local msglist = 
    {   
        account_id_ = account_id,
        job_id_ = job_id,
        player_name_ = player_name,
        slot_ = slot,
    }
    dbclient.send_create_player_to_db( _dpid, msglist )
end

function on_delete_player( _ar, _dpid, _size )
    local account_id = _ar:read_double()
    local player_id = _ar:read_ulong()
    dbclient.send_delete_player_to_db( _dpid, account_id, player_id )
end

local function add_joining_player( _dpid, _player_id, _log_info )
    local joining_player = g_joining_players[_player_id]
    if joining_player then
        if joining_player.dpid_ ~= _dpid then
            c_err( "[gamesvr](add_joining_player) dpid: 0x%X, player_id: %d, a joining player of the same player_id is already added by dpid 0x%X", _dpid, _player_id, joining_player.dpid_ ) 
            g_gamesvr:c_kick_player( _dpid )
            return false
        end
        joining_player.log_info_ = _log_info
        c_log("[gamesvr](add_joining_player) already exit, dpid: 0x%X, player_id: %d",  _dpid, _player_id)
    else
        joining_player =
        {
            dpid_ = _dpid,
            player_id_ = _player_id,
            player_data_ops_ = {},
            log_info_ = _log_info,
        }
    end
    g_joining_players[_player_id] = joining_player
    g_dpid2joining_players[_dpid] = joining_player
    c_log( "[gamesvr](add_joining_player) dpid: 0x%X, player_id: %d", _dpid, _player_id )
    return true
end

function remove_joining_player( _player_id )
    local joining_player = g_joining_players[_player_id]
    if joining_player then
        local dpid = joining_player.dpid_
        g_dpid2joining_players[dpid] = nil
        g_joining_players[_player_id] = nil
        c_log( "[gamesvr](remove_joining_player) dpid: 0x%X, player_id: %d", dpid, _player_id )
        return
    end
    c_log( "[gamesvr](remove_joining_player) player_id: %d", _player_id )
end

function remove_joining_player_by_dpid( _dpid )
    local joining_player = g_dpid2joining_players[ _dpid ]
    if joining_player then
        local player_id = joining_player.player_id_

        remove_joining_player( player_id )

        local account_info = joining_player.account_info_from_login_
        if account_info then
            -- 钻石返点
            local return_bind_diamond = account_info.return_bind_diamond_
            local return_non_bind_diamond = account_info.return_non_bind_diamond_
            mail_box_t.add_return_diamond_mail( player_id, account_info.account_id_, return_bind_diamond, return_non_bind_diamond )
        end

        for i, player_data_op in ipairs( joining_player.player_data_ops_ ) do
            local func_for_offline_player = player_data_op.func_for_offline_player_
            if func_for_offline_player then
                func_for_offline_player( player_id, player_data_op.params_ )
            end
        end
    end
    c_log( "[gamesvr](remove_joining_player_by_dpid) dpid: 0x%X", _dpid )
end

function do_join( _player_id, _account_id )
    local joining_player = g_joining_players[_player_id]
    local dpid = joining_player.dpid_
    -- 注意需要处理玩家可能跨过服的情况。如果玩家从bf回到game，那么需要等待game将存盘的包发给db才能让玩家进游戏。
    local bf_player = bfclient.g_bf_players[_player_id]
    if bf_player and bf_player.state_ == bfclient.BF_PLAYER_STATE_BF then
        -- local bf_id = bf_player.bf_id_
        -- local server_id = g_gamesvr:c_get_server_id()
        -- bfclient.remote_call_bf( bf_id, "bfsvr.on_player_join_game_with_bf_state", server_id, _player_id )
        -- send_do_join_bf_state_err( dpid )
        
        local player_data_ops = bf_player.player_data_ops_ 
        if not tisempty( player_data_ops ) then
            local lose_op_list = {}
            for data_op_id, _ in pairs( player_data_ops ) do
                tinsert( lose_op_list, data_op_id )
            end
            local lose_op_str = tconcat( lose_op_list, "," ) 
            c_err( "[gamesvr](do_join) player_id: %d, lose data operation id: %s", _player_id, lose_op_str )
        end

        bfclient.g_bf_players[_player_id] = nil
        c_err( "[gamesvr](do_join) bf_player.state_ is BF_PLAYER_STATE_BF, player_id: %d", _player_id )
        return
    end
    loginclient.send_join_to_login( dpid, _player_id, _account_id )
    dbclient.send_join_to_db( dpid, _player_id, joining_player.log_info_ )
    c_log( "[gamesvr](do_join) player_id: %d, account_id: %d, do join", _player_id, _account_id )
end

function do_join_on_recv_from_peer( _joining_player )
    local account_info = _joining_player.account_info_from_login_
    if not account_info then
        return
    end

    local player = _joining_player.player_from_db_
    if not player then
        return
    end

    local use_time = c_cpu_ms()

    local dpid = _joining_player.dpid_
    local player_id = _joining_player.player_id_
    local account_id = player.account_id_

    gamesvr.remove_joining_player( player_id )

    -- 如果玩家曾经跨服，此时需要清空该状态让玩家可以再次跨服
    local bf_player = bfclient.remove_bf_player( player_id )

    player_t.unserialize_before_join( player )
    player_t.restore_goblin_market_from_cache( player )
    player.is_prev_test_user_ = account_info.is_prev_test_user_
    player.channel_id_ = account_info.channel_id_
    player.channel_label_ = account_info.channel_label_
    player.channel_account_ = account_info.channel_account_
    player.device_name_ = account_info.device_name_ 
    player.system_name_ = account_info.system_name_ 

    if not player_mng.add_player( g_gamesvr:c_get_server_id(), player, dpid ) then
        return
    end
    player.extra_data_for_bf_.channel_account_ = account_info.channel_account_ -- 玩家在bf上时，gm在bf上查询角色信息时使用

    -- 钻石返点
    local return_bind_diamond = account_info.return_bind_diamond_
    local return_non_bind_diamond = account_info.return_non_bind_diamond_
    mail_box_t.add_return_diamond_mail( player_id, account_id, return_bind_diamond, return_non_bind_diamond )

    -- 合并银币
    local old_bind_gold_num = player.bind_gold_
    if old_bind_gold_num > 0 and player_t.consume_bind_gold( player, old_bind_gold_num, "combine_gold" ) then
        player_t.add_non_bind_gold( player, old_bind_gold_num, "combine_gold" )
    end

    if player.is_player_new_ then
        logclient.log_create( player )
        player.is_player_new_ = nil
    end
    if player.is_account_new_ then
        player.is_account_new_ = nil
    end

    logclient.log_login( player )

    if bf_player then
        for player_data_op_id, player_data_op in pairs( bf_player.player_data_ops_ ) do
            player_data_op.func_for_online_player_( player, player_data_op.params_ )
        end
    else
        player.mark_login_to_town_ = true

        -- 玩家从登陆界面 而不是从bf跨服回来
        player_t.check_notify_popup_poster_info(player)
    end

    for i, player_data_op in ipairs( _joining_player.player_data_ops_ ) do
        player_data_op.func_for_online_player_( player, player_data_op.params_ )
    end

    use_time = c_cpu_ms() - use_time

    c_log( "[gamesvr](do_join_on_recv_from_peer) dpid: 0x%X, player_id: %d, account_id: %d, return_bind_diamond: %d, return_non_bind_diamond: %d"
         , dpid, player_id, player.account_id_, return_bind_diamond, return_non_bind_diamond )

    c_prof( "[gamesvr](do_join_on_recv_from_peer) add player, player_id: %d, use time: %dms", player_id, use_time )
end

function do_rejoin( _dpid, _player_id, _bf_player, _log_info )
    if add_joining_player( _dpid, _player_id, _log_info ) then
        g_dpid_mng[_dpid] = _bf_player.dpid_info_
        do_join( _player_id, _bf_player.account_id_ )
    end
end

local function unserialize_log_info( _ar, _player_id )
    local sdk_uid = _ar:read_double()
    local sdk_machine_code = _ar:read_string()
    local sdk_channel_account = _ar:read_string()
    local sdk_channel_label = _ar:read_string()
    local sdk_p_uid = _ar:read_string()
    local sdk_devicename = _ar:read_string()
    local sdk_systemname = _ar:read_string()
    local platform_tag = _ar:read_string()
    local log_info = {
        sdk_uid_ = sdk_uid,
        sdk_machine_code_ = sdk_machine_code,
        sdk_channel_account_ = sdk_channel_account,
        sdk_channel_label_ = sdk_channel_label,
        sdk_p_uid_ = sdk_p_uid,
        sdk_devicename_ = sdk_devicename,
        sdk_systemname_ = sdk_systemname,
        platform_tag_ = platform_tag,
    }
    c_log("[gamesvr](unserialize_log_info) player_id:%d, sdk_uid:%s, sdk_machine_code:%s, sdk_channel_account:%s, sdk_channel_label:%s, sdk_p_uid:%s, sdk_devicename:%s, sdk_systemname:%s, platform_tag:%s", 
    _player_id or -1, tostring(sdk_uid), sdk_machine_code, sdk_channel_account, sdk_channel_label, sdk_p_uid, sdk_devicename, sdk_systemname, platform_tag)
    return log_info
end

function on_join( _ar, _dpid, _size )  -- 玩家上线进游戏
    local player_id = _ar:read_ulong()
    local rand_key = _ar:read_ulong()
    local log_info = unserialize_log_info( _ar, player_id )

    local dpid_info = g_dpid_mng[_dpid]
    local account_info = dpid_info.account_info_
    if not account_info then
        c_err( "[gamesvr](on_join) no account info of dpid: 0x%X", _dpid )
        return
    end

    if account_info.rand_key_ ~= rand_key then
        c_login( "[db_login](do_player_join) wrong rand_key! player_id: %d, account_id: %d, expect rand_key: %d, received rand_key: %d"
               , player_id, account_info.account_id_, account_info.rand_key_, rand_key ) 
        return
    end

    if add_joining_player( _dpid, player_id, log_info ) then
        do_join( player_id, account_info.account_id_ )
    end
    c_log( "[gamesvr](on_join) dpid: 0x%X, player_id: %d", _dpid, player_id )
end

function on_rejoin( _ar, _dpid, _size )  -- 玩家按流程从bf回原服
    local player_id = _ar:read_ulong()
    local rejoin_certify_code = _ar:read_int()
    local log_info = unserialize_log_info( _ar, player_id )

    local bf_player = bfclient.g_bf_players[player_id]
    if not bf_player then
        c_err( "[gamesvr](on_rejoin) dpid: 0x%X, there's no bf_player of player_id %d", _dpid, player_id )
        g_gamesvr:c_kick_player( _dpid )
        return
    end

    if rejoin_certify_code ~= bf_player.rejoin_certify_code_ then
        c_err( "[gamesvr](on_rejoin) dpid: 0x%X, player_id:%d, rejoin_certify_code not match, expected: %d, received: %d",
                _dpid, player_id, bf_player.rejoin_certify_code_, rejoin_certify_code )
        g_gamesvr:c_kick_player( _dpid )
        return
    end

    c_log( "[gamesvr](on_rejoin) dpid: 0x%X, player_id: %d, rejoin_certify_code: %d", _dpid, player_id, rejoin_certify_code )   
    login_mng.on_migrated_player_rejoin( _dpid, bf_player.account_id_, rejoin_certify_code )
    do_rejoin( _dpid, player_id, bf_player, log_info )
end

function on_quit_login_queue( _ar, _dpid, _size )
    login_mng.account_quit_login_queue( _dpid )
end

function on_request_session_alive( _ar, _dpid, _size )
    local account_id = _ar:read_double()
    local certify_key = _ar:read_int()
    login_mng.request_session_alive( _dpid, account_id, certify_key )
end

function on_reselect_player( _ar, _dpid, _size )
    local account_id = _ar:read_double()
    local certify_key = _ar:read_int()
    local player_id = _ar:read_int()
    login_mng.return_to_select_player( _dpid, account_id, certify_key, player_id )
end

function on_client_connect( _ar, _dpid, _size )
    g_dpid_mng[_dpid] = {}
    login_mng.create_connection( _dpid )
    send_server_info( _dpid )
end

function on_client_disconnect( _ar, _dpid, _size )
    if not g_mini_server then
        team_inst_gs.on_player_disconnect(_dpid)
        login_mng.close_connection( _dpid )    
        on_req_honor_ladder_quit_match( _ar, _dpid, _size )
    end
end

function clear_dpid_info( _dpid )
    local dpid_info = g_dpid_mng[ _dpid ]
    if dpid_info then
        local account_info = dpid_info.account_info_
        if account_info then
            local account_name = account_info.account_name_
            -- 需要考虑顶号的情况，可能老连接断开时新连接已经登录完毕
            if account_name and g_account_name_2_dpid[ account_name ] == _dpid then
                g_account_name_2_dpid[ account_name ] = nil
            end 
        end
    end
    g_dpid_mng[ _dpid ] = nil
end

---------------------------------
-- _dpid is as the 1st argument of 
-- the method open to the client 
---------------------------------
client_white_func_table = {
}

function on_client_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args = _ar:read_string()
    if (client_white_func_table[func_name] == nil) then
        c_err( sformat( "invalid function name:[%s]", func_name ) )
        return
    end

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "(on_client_remote_call)func %s not found", func_name ) )
        return 
    end

    local arg_func, msg = loadstring( args )
    if not arg_func then 
        c_err( 0 )( sformat( "(on_client_remote_call)func %s, args: %s, error: %s", 
            func_name, args, msg ) )
        return
    end  
    func( _dpid, unpack( arg_func() ) )
end

--
-- RPC函数白名单
--
game_server_white_func_table = {
    ["plane_t.do_give_player_plane_drop"]=1,
    ["instance_t.do_give_player_plane_drop"]=1,
    ["instance_mng.quit_instance"]=1,
    ["drop.on_miniserver_add_drop"]=1,
    ["player_t.use_potion_process"]=1,
    ["player_t.do_use_revive_item"]=1,
    ["player_t.revert_skills"]=1,
    ["buff_effect.set_client_avatar"]=1,
    ["buff_effect.reset_client_avatar"]=1,
    ["player_t.rpc_consume_time_ctrl"]=1,
    ["tamriel_relic.tamriel_give_miniserver_mode_award"]=1,
    ["tamriel_relic.consume_energy_robot_mode"]=1,
    ["honor_hall.honor_hall_do_consume_time_ctrl"] = 1, 
    ["honor_hall.honor_hall_do_give_miniserver_award"] = 1,
    ["player_t.update_tower_progress"] = 1,
    ["bulletin_mng.miniserver_template_bulletin"]=1,
    ["monster_t.fuck_hacker"]=1,
	["player_t.miniserver_remote_set_stage_pass_star"]=1,
    ["player_t.update_palace_progress"]=1,
    ["player_t.palace_select_model"]=1,
    ["player_t.save_palace_boss_data_from_miniserver"]=1,
    ["player_t.save_palace_player_data_from_miniserver"]=1,
    ["player_t.save_palace_old_main_avatar_from_miniserver"]=1,
    ["player_t.recover_palace_old_main_avatar"]=1,
    ["player_t.save_palace_boss_hp_from_miniserver"]=1,
}

--
-- game server 处理来自mini server 的RPC请求
--
function on_mini_server_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args = _ar:read_buffer()

    if (game_server_white_func_table[func_name] == nil) then
        c_err( sformat( "invalid function name:[%s]", func_name ) )
        return
    end

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "(on_mini_server_remote_call)func %s not found", func_name ) )
        return 
    end

    local args_table = c_msgunpack( args )
    func( _dpid, unpack( args_table ) )
end


--
-- mini server 处理来自game server 的RPC请求
--
function on_game_server_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args = _ar:read_buffer()

    --[[
    if (client_white_func_table[func_name] == nil) then
        c_err( sformat( "invalid function name:[%s]", func_name ) )
        return
    end
    ]]

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "(on_game_server_remote_call)func %s not found", func_name ) )
        return 
    end

    local args_table = c_msgunpack( args )
    func( _dpid, unpack( args_table ) )
end

function on_mini_server_create_inst( _ar, _dpid, _size )
    local create_param = instance_t.unserialize_create_instance_param( _ar )
    instance_mng.miniserver_do_player_join_activity_instance( _dpid, create_param )
end 

function on_client_prepare_spawn_data_id( _ar , _dpid , _size )
    local inst_len = _ar:read_int()
    if inst_len > 5 then return end  

    local inst_id_list = {}
    for i =1, inst_len do 
        local inst_id = _ar:read_int()
        tinsert( inst_id_list, inst_id ) 
    end
    instance_mng.set_miniserver_instance_spawn_datat_id( inst_id_list )
end 

--
--　副本总入口,不管单机还是联网都从这里进来校验
--
function on_req_gameserver_join_activity_instance( _ar , _dpid , _size )
    local log_head = sformat("%s(on_req_gameserver_join_activity_instance)", LOG_HEAD)
    local param = {}

    local xor_activity_inst_id = _ar:read_int()
    local xor_activity_type = _ar:read_int()
    local xor_extra_param = _ar:read_int()
    local xor_client_time = _ar:read_int()
    local xor_inst_sign = _ar:read_int()
    local inst_secret = _ar:read_int()

    param.activity_inst_id_ = c_xor( xor_activity_inst_id, inst_secret )
    param.enter_type_       = c_xor( xor_activity_type, inst_secret )
    param.extra_param_      = c_xor( xor_extra_param, inst_secret )
    param.client_time_      = c_xor( xor_client_time, inst_secret ) 
    param.client_inst_sign_ = c_xor( xor_inst_sign, inst_secret )
    param.client_inst_secret_ = inst_secret

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( sformat( "[gamesvr](on_req_gameserver_join_activity_instance)can't get player by dpid: 0x%X", _dpid ) )
        return
    end

    --[[
    if player:is_team_matching() then 
        c_log( "[gamesvr](on_req_gameserver_join_activity_instance)player:%s is in team match", player.tostr_ )
    else
        param.player_ = player
        instance_mng.gameserver_do_player_join_activity_instance( param )
    end
    --]]
    
    param.player_ = player
    instance_mng.gameserver_do_player_join_activity_instance( param )
end 

function on_request_rejoin_activity_instance( _ar, _dpid, _size )
    c_log("[gamesvr](on_request_rejoin_activity_instance) reject rejoin, kick dpid: 0x%X", _dpid )
end

function on_consume_instance_cost( _ar, _dpid, _size )
end

function on_get_secret_and_time( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        return
    end
    player:add_server_time()
    player:add_skill_secret( player.skill_secret_ )
end

function on_confirm_quit_instance( _ar, _dpid, _size )
    local inst_id = _ar:read_int()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        c_trace("[gamesvr](on_confirm_quit_instance)can't find player by dpid: 0x%X, inst_id: %d", _dpid, inst_id)
        return
    end

    if player:is_in_guild_scene() then
        guild_scene_mng.on_player_quit_battlefield( player, true ) 
    else
        instance_mng.check_quit_instance( player )
    end
end


function on_reconfirm_quit_instance( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        instance_mng.gameserver_requit_instance( player )
    end
end

function on_skill_slots( _ar, _dpid, _size )
    local slot = _ar:read_byte()
    local skill_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:set_skill_slot( slot, skill_id )
    end
end

function on_skill_level_up( _ar, _dpid, _size )
    local skill_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:do_skill_level_up( skill_id )
    end
end

function on_learn_talent( _ar, _dpid, _size )
    local talent_upgrade_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:do_learn_talent( talent_upgrade_id )
    end
end

function on_manual_up_level(_ar , _dpid , _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player:try_up_level() then 
        player:manual_up_level()
        gamesvr.send_manual_up_level_ack( _dpid , 0 )
    else
        gamesvr.send_manual_up_level_ack( _dpid , 1)
    end 
end 

function on_req_quest_heianzhimen_info( _ar , _dpid , _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 
    player:add_ack_quest_heianzhimen_info()
end

function on_req_quest_heianzhimen_start_new ( _ar , _dpid , _size )
    local diamond_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    quest_heianzhimen_t.hazm_start_new( player, diamond_index )
end

function on_req_quest_heianzhimen_open_random_card( _ar , _dpid , _size )
    local card_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    quest_heianzhimen_t.hazm_on_open_random_card( player, card_index )
end

function on_req_quest_heianzhimen_select_diamond_index( _ar, _dpid, _size )
end 

function on_req_quest_heianzhimen_click_step_next( _ar , _dpid , _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 
    quest_heianzhimen_t.hazm_on_client_click_step_next( player )
end

function on_req_quest_heianzhimen_reset_count( _ar, _dpid , _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    quest_heianzhimen_t.hazm_reset_quest(  player )
end 

function on_req_quest_mammons_treasure_reset( _ar , _dpid , _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    -- quest_mammons_treasure.handle_reset( player )
end

function on_req_join_honor_hall_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    local mode = _ar:read_int()
    if player then
        honor_hall.honor_hall_join_match( player, mode )
    end
end

function on_honor_hall_invite_pk( _ar, _dpid, _size )
    local receiver_server_id = _ar:read_int() 
    local receiver_player_id = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    honor_hall.on_honor_hall_invite_pk( player, receiver_server_id, receiver_player_id )
end

function on_honor_hall_ack_pk( _ar, _dpid, _size )
    local ack = _ar:read_byte()
    local inviter_player_id = _ar:read_int()
    local inviter_server_id = _ar:read_int()
    local inviter_name = _ar:read_string()
    local inviter_level = _ar:read_int()
    local inviter_model_id = _ar:read_int()
    local inviter_gs = _ar:read_int()

    local receiver = player_mng.get_player_by_dpid( _dpid )
    if not receiver then
        return
    end

    if receiver:player_current_match_type() ~= share_const.MATCH_NULL then
        receiver:add_defined_text( MESSAGES.CASTLE_INVITE_FALSE )
        return
    end

    if ack == 1 then
        honor_hall.on_honor_hall_ack_pk( receiver, inviter_player_id, inviter_server_id, inviter_name, inviter_level, inviter_model_id, inviter_gs )
    else
        honor_hall.on_honor_hall_refuse_pk( receiver, inviter_player_id, inviter_server_id, inviter_name, inviter_level, inviter_model_id, inviter_gs )
    end
end

function on_req_quit_honor_hall_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    local mode = _ar:read_int()
    if player then
        honor_hall.honor_hall_quit_match( player, mode )
    end
end

function on_req_honor_hall_rank( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    local client_today_index = _ar:read_int()
    if player then
        honor_hall.honor_hall_send_rank( player, client_today_index )
        honor_hall.honor_hall_send_player_score( player )
    end
end

function on_req_honor_hall_get_award( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_hall.honor_hall_on_player_get_award( player )
    end
end

function on_req_honor_ladder_matching_state( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_game.on_client_req_ladder_matching_state( player.player_id_ )
end 

function on_req_honor_ladder_join_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_game.on_client_req_join_ladder_match( player.player_id_ )
end 

function on_req_honor_ladder_quit_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_game.on_client_req_quit_ladder_match( player )
end 

function on_req_honor_ladder_cancel( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_game.on_client_req_cancel_ladder_match( player.player_id_ )
end 

function on_req_honor_ladder_confirm( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_game.on_client_req_confirm_ladder_match( player.player_id_ )
end 

function on_req_honor_ladder_my_rank( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_game.on_client_req_ladder_my_rank( player.player_id_ )
end 

function on_event_honor_tour_sign_up( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_tour_game.on_client_sign_up_honor_tour( player )
    end
end

function on_event_honor_tour_join_bf( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_tour_game.honor_tour_player_join_bf( player )
    end
end

function on_event_honor_tour_qualification_player_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_tour_game.honor_tour_on_client_get_qualification_player_list( player )
    end
end


function on_join_battlefield_match( _ar, _dpid, _size )
    local mode = _ar:read_ubyte()
    local model_index = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    if mode == tamriel_relic.GAME_MODE_ROBOT then
        tamriel_relic_robot.on_player_start_robot_game( player )
    elseif mode == tamriel_relic.GAME_MODE_PVP
        or mode == tamriel_relic.GAME_MODE_PVP_FIXED_TIME then 
        tamriel_relic_game.player_sign_up( player, mode, model_index ) 
    end
end

function on_quit_battlefield_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        tamriel_relic_game.player_cancel_sign( player )  
    end
end

function on_req_battle_field_status( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 
  
    tamriel_relic.tamriel_send_player_rejoin_status( player )
end

function on_req_rejoin_battle_field( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 
    tamriel_relic.tamriel_send_player_to_bf_again( player )
end

function on_tamriel_player_join( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    tamriel_relic.on_player_join_battle( player )
end

function on_join_five_pvp_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    five_pvp_match_gs.join_match( player )
end

function on_quit_five_pvp_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    five_pvp_match_gs.quit_match( player )
end

function on_req_rejoin_five_pvp( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 
    five_pvp_match_gs.player_join_bf( player )
end

---------------------------------------------------------------------------------------
if g_mini_server then

function on_game_gmcmd( _dpid, _gm_str )
    local func = loadstring( _gm_str )
    if not func then
        c_err( sformat( "[gamesvr](on_game_gmcmd)load func error. gmstr: %s", _gm_str ) )
        return
    end
    func()
    c_log( sformat( "[gamesvr](on_game_gmcmd)gmstr: %s called succ", _gm_str ) )
end

function on_client_create_player(_ar, _dpid, _size)
    local dpid = _ar:read_int()
    local ctrl_id = _ar:read_int()
    local scene_id = _ar:read_int()
    local player_id = _ar:read_int()
    local player_name = _ar:read_string()
    local job_id = _ar:read_int()
    local level = _ar:read_int()
    local model_id = _ar:read_int()
    local x, z, angle = _ar:read_compress_pos_angle()
    local player = 
    { 
        account_id_ = 0,
        player_id_ = player_id, 
        player_name_ = player_name,
        scene_id_ = scene_id, 
        job_id_ = job_id,
        level_ = level,
        model_id_ = model_id,
        exp1_ = 0,
        exp2_ = 0,
        x_ = x,
        y_ = 0,
        z_ = z,
        angle_ = angle,
        job_skills_ = {},
    }
    player_mng.add_player( MINI_SERVER_ID, player, dpid, ctrl_id )
end

function on_client_create_monster(_ar, _dpid, _size)
    local scene_id = _ar:read_int()
    local type_id = _ar:read_int()
    local x, z, angle = _ar:read_compress_pos_angle()
    c_log( "[mini_gamesvr](on_client_create_monster) scene_id: %d, type_id: %d", scene_id, type_id ) 
    monster_t.add_monster( type_id, 1, scene_id, x, z, angle , nil, 0, 0 )
end

function on_client_delete_player( _ar, _dpid, _size )
    local player_id = _ar:read_int()
    player_mng.del_player( MINI_SERVER_ID, player_id )
    spirit_t.g_buff_sn_map = {} -- del buff sn map while player offline
    c_log( "[mini_gamesvr](on_client_delete_player) player_id: %d", player_id ) 
end

function on_client_replace_dpid( _ar, _dpid, _size )
    local player_id = _ar:read_int()
    local new_dpid = _ar:read_int()

    local player = player_mng.get_player( MINI_SERVER_ID, player_id )
    if player then
        local old_dpid = player.dpid_
        if old_dpid ~= new_dpid then
            g_playermng:c_remove_player( old_dpid )
            g_playermng:c_add_player( new_dpid, player.core_ )
            player.dpid_ = new_dpid
            player_mng.g_player_dpid_map[ new_dpid ] = player
            player_mng.g_player_dpid_map[ old_dpid ] = nil
            c_log( "[gamesvr](on_client_replace_dpid) player_id: %d, old_dpid: 0x%X, new_dpid: 0x%X", player_id, old_dpid, new_dpid )
        else
            c_err( "[gamesvr](on_client_replace_dpid) player_id: %d, old_dpid and new_dpid are equal 0x%X", player_id, new_dpid )
        end
    end
end

function on_client_sync_attribute(_ar, _dpid, _size)
    local obj_id = _ar:read_int()
    local index = _ar:read_int()
    local _, read_func = player_attr.get_attr_send_func( index )
    local value = read_func( _ar )

    local obj = ctrl_mng.get_ctrl( obj_id )
    if not obj then
        c_err( "[gamesvr](on_client_sync_attribute) obj not found, obj_id: %d", obj_id )
        return
    end
    local attr_name = nil
    for k, v in pairs( JOBS.JOB_ATTR_INDEX ) do
        if v == index then
            attr_name = k
            break
        end
    end
    if attr_name then
        obj[attr_name] = value
    end
end

function on_client_sync_skill(_ar, _dpid, _size)
    local obj_id = _ar:read_int()
    local skill_id = _ar:read_int()
    local skill_level = _ar:read_int()
    local key_skill = _ar:read_int()

    local skill_info = { level_ = skill_level, key_skill_ = key_skill }

    local obj = ctrl_mng.get_ctrl( obj_id )
    if not obj then
        c_err( "[gamesvr](on_client_sync_skill) obj not found, obj_id: %d", obj_id )
        return
    end
    if not obj.job_skills_ then
        obj.job_skills_ = { }
    end
    obj.job_skills_[skill_id] = skill_info
end

function on_client_sync_add_buff(_ar, _dpid, _size)
    local obj_id = _ar:read_int()
    local buff_sn = _ar:read_int()
    local buff_id = _ar:read_int()
    local skill_id = _ar:read_int()
    local attr_count = _ar:read_int()
    local attrs = {}
    for i = 1, attr_count, 1 do
        local attr_index = _ar:read_byte()
        local delta_value = _ar:read_float()
        attrs[attr_index] = delta_value
    end
    local equip_attr_index = _ar:read_byte()
    local equip_delta_type = _ar:read_byte()

    local obj = ctrl_mng.get_ctrl( obj_id )
    if not obj then
        c_err( "[gamesvr](on_client_sync_add_buff) obj not found, obj_id: %d", obj_id )
        return
    end
    obj:on_sync_add_buff( buff_sn, buff_id, skill_id, attrs, equip_attr_index, equip_delta_type )
end

function on_client_sync_remove_buff(_ar, _dpid, _size)
    local obj_id = _ar:read_int()
    local buff_num = _ar:read_int()
    local sns = {}
    for i = 1, buff_num, 1 do
        local sn = _ar:read_int()
        tinsert( sns, sn )
    end
    local obj = ctrl_mng.get_ctrl( obj_id )
    if obj then
        obj:on_sync_remove_buff( sns )
    end
end

function on_clear_all_synced_buff(_ar, _dpid, _size )
    local obj_id = _ar:read_int()
    local obj = ctrl_mng.get_ctrl( obj_id )
    if obj then
        obj:clear_all_synced_buff()
    end
end

function on_client_sync_city_pos(_ar, _dpid, _size)
    local obj_id = _ar:read_int()
    local x = _ar:read_float()
    local z = _ar:read_float()
    local obj = ctrl_mng.get_ctrl( obj_id )

    if not obj then
        c_err( "[gamesvr](on_client_sync_city_pos) obj not found, obj_id: %d", obj_id )
        return
    end

    if obj.scene_id_ ~= channel_mng.CITY_SCENE_ID then
        c_err( "[gamesvr](on_client_sync_city_pos) obj not in city scene, obj_id: %d, scene_id: %d", obj_id, obj.scene_id_ )
        return
    end

    obj.core_:c_setpos( x, 0, z )
end
end  -- g_mini_server
--------------------------------------------------------------------------------------------------------


function on_player_rune_active( _ar, _dpid, _size )
    local rune_cursor = _ar:read_int()
    local index = _ar:read_byte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_active_player_rune( rune_cursor, index )
end

function on_player_rune_awake( _ar, _dpid, _size )    
    local rune_rank_id = _ar:read_byte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_awake_player_rune( rune_rank_id )
end

function on_player_soul_awake( _ar, _dpid, _size )
    local soul_index = _ar:read_ubyte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    player:on_player_soul_awake( soul_index )
end

function on_player_soul_change_avatar( _ar, _dpid, _size )
    local soul_index = _ar:read_ubyte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_player_soul_change_avatar( soul_index )
end

function on_player_avatar_change_main( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_change_main_avatar( avatar_index )
end

function on_player_avatar_change_light( _ar, _dpid, _size )
    local new_avatar_index = _ar:read_ubyte()
    local old_avatar_index = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    player:on_avatar_change_light_avatar( new_avatar_index, old_avatar_index )
end

function on_player_avatar_ascend_level( _ar, _dpid, _size )  
    local avatar_index = _ar:read_ubyte()
    local item_index = _ar:read_ubyte()
    local auto = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    if auto > 0 then
        player:on_avatar_ascend_level_auto( avatar_index, item_index )
    else
        player:on_avatar_ascend_level( avatar_index, item_index )
    end
end

function on_player_avatar_ascend_star( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_ascend_star( avatar_index )
end

function on_player_avatar_change_artifact( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local artifact_id = _ar:read_int()
    local old_item_index = _ar:read_int()
    local new_item_index = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_change_artifact( avatar_index, artifact_id, old_item_index, new_item_index )
end

function on_player_fusion_artifact( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local artifact_id = _ar:read_int()
    local item_index = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_fusion_artifact( avatar_index, artifact_id, item_index )
end

function on_player_avatar_ascend_med_star( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local med_index = _ar:read_ubyte()
    local item_index = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_ascend_meditation_star( avatar_index, med_index, item_index )
end

function on_player_avatar_reborn_med_skill( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local med_index = _ar:read_ubyte()
    local item_index = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_reborn_meditation_skill( avatar_index, med_index, item_index )
end

function on_player_avatar_confirm_med_skill( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local med_index = _ar:read_ubyte()
    local accept = _ar:read_ubyte() > 0

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_confirm_meditation_skill( avatar_index, med_index, accept )
end

function on_player_avatar_destroy( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_destroy_avatar( avatar_index )
end

--add by kent
function on_player_avatar_ascend_rank( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local rank_up_idx = _ar:read_ubyte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_avatar_ascend_rank( avatar_index, rank_up_idx)
end

function on_player_astrolabe_ascend_level( _ar, _dpid, _size )
    local astrolabe_index = _ar:read_ubyte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_astrolabe_ascend_level( astrolabe_index )
end

function on_player_astrolabe_oper_avatar( _ar, _dpid, _size )
    local astrolabe_index = _ar:read_ubyte()
    local avatar_index = _ar:read_ubyte()
    local oper = _ar:read_ubyte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:on_astrolabe_oper_avatar( avatar_index, astrolabe_index, oper )
end
--

function on_player_avatar_material_conversion( _ar, _dpid, _size )
    local item_index = _ar:read_int()
    local item_id = _ar:read_int()
    local item_num = _ar:read_int()
    local convert_num = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    local succ, item = check_index_item( item_id, item_num, item_index, player, "avatar_conversion" )
    if not succ then return end

    if item_num < convert_num then
        c_err( "[gamesvr](on_player_avatar_material_conversion) player_id: %d, item_id: %d, item_num: %d, convert_num: %d", player.player_id_, item_id, item_num, convert_num )
        return
    end

    player:on_item_conversion( item_index, item_id, convert_num )
end

function on_player_buy_energy( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:buy_energy()
end

function on_send_migrate_begin_succ( _ar, _dpid, _size )
    bfclient.on_player_send_migrate_begin_succ( _dpid )
end

function on_goblin_market_redraw_all( _ar, _dpid, _size )
    local serial_num = _ar:read_uint()
    local honor_consumption = _ar:read_ushort()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_goblin_market_redraw_all( serial_num, honor_consumption )
end

function on_goblin_market_request_update( _ar, _dpid, _size )
    local serial_num = _ar:read_uint()
    local honor_consumption = _ar:read_ushort()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_goblin_market_require_update( serial_num, honor_consumption )
end


function on_read_mail( _ar, _dpid, _size )
    local mail_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:read_mail( mail_index )
end

function on_pick_mail( _ar, _dpid, _size)
    local mail_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:pick_mail( mail_index )
end

function on_pray_by_item( _ar, _dpid, _size )
    local serial_num = _ar:read_uint()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_pray_by_item( serial_num )
end

function on_pray_one_by_diamond( _ar, _dpid, _size )
    local serial_num = _ar:read_uint()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_pray_one_by_diamond( serial_num )
end

function on_pray_ten_by_diamond( _ar, _dpid, _size )
    local serial_num = _ar:read_uint()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_pray_ten_by_diamond( serial_num )
end

function on_pray_check_need_update( _ar, _dpid, _size )
    local serial_num = _ar:read_uint()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_pray_check_need_update( serial_num )
end

function on_new_recharge_request( _ar, _dpid, _size )
    local product_id = _ar:read_int()
    local order_id = _ar:read_double()
    local channel_label = _ar:read_string()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        c_err( "[gamesvr](on_new_recharge_request)dpid: %d, product_id: %d, order_id: %d. player offline", _dpid, product_id, order_id )
        return 
    end

    if splus_verify.is_sdk_id_out_of_range( order_id ) then
        c_err( "[gamesvr](on_new_recharge_request)dpid: %d, product_id: %d, order_id: %d. order_id out of range", _dpid, product_id, order_id )
        return
    end

    player:begin_recharge( product_id, order_id, channel_label )
end

function on_cancel_recharge_request( _ar, _dpid, _size )
    local order_id = _ar:read_double()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        c_err( "[gamesvr](on_cancel_recharge_request)dpid: %d, order_id: %d. player offline", _dpid, order_id )
        return 
    end

    if splus_verify.is_sdk_id_out_of_range( order_id ) then
        c_err( "[gamesvr](on_cancel_recharge_request)dpid: %d, order_id: %d. order_id out of range", _dpid, order_id )
        return
    end

    player:cancel_recharge( order_id )
end

function on_complete_recharge_request( _ar, _dpid, _size )
    local order_id = _ar:read_double()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        c_err( "[gamesvr](on_complete_recharge_request)dpid: %d, order_id: %d. player offline", _dpid, order_id )
        return 
    end

    if splus_verify.is_sdk_id_out_of_range( order_id ) then
        c_err( "[gamesvr](on_complete_recharge_request)dpid: %d, order_id: %d. order_id out of range", _dpid, order_id )
        return
    end

    player:complete_recharge( order_id )
end

function on_continue_recharge_request( _ar, _dpid, _size )
    local product_id = _ar:read_int()
    local order_id = _ar:read_double()
    local channel_label = _ar:read_string()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        c_err( "[gamesvr](on_continue_recharge_request)dpid: %d, product_id: %d, order_id: %d. player offline", _dpid, product_id, order_id )
        return 
    end

    if splus_verify.is_sdk_id_out_of_range( order_id ) then
        c_err( "[gamesvr](on_continue_recharge_request)dpid: %d, product_id: %d, order_id: %d. order_id out of range", _dpid, product_id, order_id )
        return
    end

    player:continue_uncompleted_recharge( product_id, order_id, channel_label )
end

function on_buy_privilege_box( _ar, _dpid, _size )
    local vip_config_id = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_buy_privilege_box( vip_config_id )
end

function on_req_instance_sweep( _ar, _dpid, _size )
    local activity_instance_id  = _ar:read_int()
    local enter_type   = _ar:read_int()
    local param = {}
    param.p1_ = _ar:read_int()
    param.p2_ = _ar:read_int()
    param.p3_ = _ar:read_int()
    param.p4_ = _ar:read_int()
    param.sweep_type_ = _ar:read_ubyte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    instance_t.do_instance_sweep( player , activity_instance_id , enter_type, param )   
end

function on_req_plane_drop_again( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    instance_t.on_req_give_plane_drop_again( player )
end

function on_finish_main_quest( _ar, _dpid, _size )
    local quest_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_finish_main_quest( quest_id )
end

function on_exchange_purple_soul( _ar, _dpid, _size )
    local soul_list_idx = _ar:read_int()
    local exchange_num = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_exchange_purple_soul( soul_list_idx, exchange_num )
    end
end

function on_get_coupon( _ar, _dpid, _size)
    local coupon_id = _ar:read_string()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_coupon) cant not found player by dpid: 0x%X", _dpid)
        return
    end
    coupon_mng.on_player_get_coupon( player, coupon_id)
end

function on_change_newbie_guide_state_to_finish( _ar, _dpid, _size )
    local msg_list_index = _ar:read_int()
    local id = _ar:read_int()
    local is_group = _ar:read_ubyte()
    local finish_type = _ar:read_int()
    local is_finish_permanent = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_change_newbie_guide_state_to_finish) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:on_change_newbie_guide_state_to_finish( id, is_group, finish_type, is_finish_permanent )
    player:add_ack_recv_newbie_guide_msg( msg_list_index )
end

function on_notify_newbie_guide_current_progress( _ar, _dpid, _size )
    local msg_list_index = _ar:read_int()
    local guide_id = _ar:read_int()
    local key_sub_part_finish = _ar:read_ubyte()
    local is_started = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_notify_newbie_guide_current_progress) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:on_notify_newbie_guide_current_state( guide_id, key_sub_part_finish, is_started )
    player:add_ack_recv_newbie_guide_msg( msg_list_index )
end

function on_referesh_newbie_guide_history_cond_rec( _ar, _dpid, _size )
    local msg_list_index = _ar:read_int()
    local guide_id = _ar:read_int()
    local cond_id = _ar:read_int()
    local cond_val_num = _ar:read_int()
    local cur_vals = {}
    for i = 1, cond_val_num do
        tinsert( cur_vals, _ar:read_int())
    end
    local is_met = _ar:read_ubyte()
    local cond_rec = 
    {
        cur_vals_ = cur_vals,
        is_met_ = is_met,
    }

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_referesh_newbie_guide_history_cond_rec) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:on_refresh_newbie_guide_history_condition_record( guide_id, cond_id, cond_rec )
    player:add_ack_recv_newbie_guide_msg( msg_list_index )
end

function on_suspend_mst_ai_in_instance( _ar, _dpid, _size )
    local is_suspend = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_suspend_mst_ai_in_instance) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:on_suspend_mst_ai_in_instance( is_suspend )
end

function on_player_buy_mystic_goods( _ar, _dpid, _size )
    local dealer_type = _ar:read_byte()
    local index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_player_buy_mystic_goods) can not find player by dpid: 0x%X", _dpid )
        return
    end
    player:on_buy_mystic_goods( dealer_type, index )
end

function on_player_refresh_mystic( _ar, _dpid, _size )
    local dealer_type = _ar:read_byte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_player_refresh_mystic) can not find player by dpid: 0x%X", _dpid )
        return
    end
    player:on_refresh_mystic( dealer_type )
end

function on_check_need_update_mystic( _ar, _dpid, _size )
    local dealer_type = _ar:read_byte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamsvr](on_check_need_update_mystic) can not find player by dpid: 0x%X", _dpid )
        return
    end
    player:check_need_update_mystic( dealer_type )
end

function on_event_ares_sign_up( _ar, _dpid, _size )
    local model_count = _ar:read_byte()
    local model_list = {}
    for i = 1, model_count do
        model_list[ i ] = _ar:read_byte()
    end
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_ares_game.player_sign_up( player, model_list )
    end
end

function on_event_ares_join_bf( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_ares_game.player_join_bf( player )
    end
end

function on_ares_witness_request_matches( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        ares_witness_game.on_player_request_matches( player ) 
    end
end

function on_ares_witness_add_observe( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        ares_witness_game.on_player_add_observe( player )
    end
end

function on_ares_witness_request_witness( _ar, _dpid, _size )
    local version = _ar:read_int()
    local match_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        ares_witness_game.on_player_request_witness( player, version, match_sn ) 
    end
end

function on_honor_witness_request_matches( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_witness_game.on_player_request_matches( player ) 
    end
end

function on_honor_witness_add_observe( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_witness_game.on_player_add_observe( player )
    end
end

function on_honor_witness_request_witness( _ar, _dpid, _size )
    local version = _ar:read_int()
    local match_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_witness_game.on_player_request_witness( player, version, match_sn ) 
    end
end


if not g_mini_server then

local function check_system_unlock_player_shop( _player )
    if not system_unlock_mgr.is_unlocked( system_unlock_mgr.SYSTEM_UNLOCK_ID_TRADE_SYSTEM ) then
        c_err( "[gamesvr](check_system_unlock_player_shop) not passed, player_id: %d, player_level: %d", _player.player_id_, _player.level_ )
        return false
    end
    return true
end

function on_create_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_type = _ar:read_byte()
    local shop_name = sgsub( _ar:read_string(), "[\r\n]", "" )
    local shop_desc = sgsub( _ar:read_string(), "[\r\n]", "" )
    player:create_shop( shop_type, shop_name, shop_desc )
end

function on_put_items_into_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end

    local shop_local_id = _ar:read_int()

    local item_cnt = _ar:read_byte()
    if item_cnt > share_const.SHOP_PUT_ITEM_CNT_MAX then
        c_err( "[gamesvr](on_put_items_into_shop) item count is too big, item count: %d, limit: %d", item_cnt, share_const.SHOP_PUT_ITEM_CNT_MAX )
        return
    end

    local items = {}
    for i = 1, item_cnt do
        local item =
        {
            index_ = _ar:read_int(),
            item_id_ = _ar:read_int(),
            item_num_ = _ar:read_int(),
            price_ = _ar:read_int(),
        }
        tinsert( items, item )
    end

    player:put_items_into_shop( shop_local_id, items )
end

function on_get_shop_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_type = _ar:read_byte()
    local sort_type = _ar:read_byte()
    local page = sort_type == share_const.SHOP_LIST_SORTED and _ar:read_int() or 1
    player:get_shop_list( shop_type, sort_type, page )
end

function on_get_favorite_shops( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    player:get_favorite_shops()
end

function on_get_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_server_id = _ar:read_int()
    local shop_local_id = _ar:read_int()
    player:get_shop( shop_server_id, shop_local_id )
end

function on_buy_item_in_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_server_id = _ar:read_int()
    local shop_local_id = _ar:read_int()
    local item_sn_in_shop = _ar:read_double()
    local item_id = _ar:read_int()
    local item_num = _ar:read_int()
    player:buy_item_in_shop( shop_server_id, shop_local_id, item_sn_in_shop, item_id, item_num )
end

function on_withdraw_shop_gold( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    local gold = _ar:read_int()
    player:withdraw_shop_gold( shop_local_id, gold )
end

function on_put_gold_into_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    local gold = _ar:read_int()
    player:put_gold_into_shop( shop_local_id, gold )
end

function on_withdraw_shop_item( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    local item_sn_in_shop = _ar:read_double()
    player:withdraw_shop_item( shop_local_id, item_sn_in_shop )
end

function on_add_shop_bid_gold( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    local gold = _ar:read_int()
    player:add_shop_bid_gold( shop_local_id, gold )
end

function on_get_shop_bid_gold_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    player:get_shop_bid_gold_info( shop_local_id )
end

function on_add_favorite_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_server_id = _ar:read_int()
    local shop_local_id = _ar:read_int()
    player:add_favorite_shop( shop_server_id, shop_local_id )
end

function on_rm_favorite_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_server_id = _ar:read_int()
    local shop_local_id = _ar:read_int()
    player:remove_favorite_shop( shop_server_id, shop_local_id )
end

function on_modify_shop_name_desc( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    local shop_name = _ar:read_string()
    local shop_desc = _ar:read_string()
    player:modify_shop_name_desc( shop_local_id, shop_name, shop_desc )
end

function on_modify_shop_item_price( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    local item_sn_in_shop = _ar:read_double()
    local price = _ar:read_int()
    player:modify_shop_item_price( shop_local_id, item_sn_in_shop, price )
end

function on_close_shop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    if not check_system_unlock_player_shop( player ) then return end
    local shop_local_id = _ar:read_int()
    player:close_shop( shop_local_id )
end

end


function on_put_items_into_warehouse( _ar, _dpid, _size )
    local item_count = _ar:read_byte()
    if item_count <= 0 or item_count > bag_t.MAX_PUT_IN_WAREHOUSE_NUM_EACH_TIME then return end

    local item_list = {}
    local item_index_list = {}
    for i = 1, item_count do
        local item_info = {}
        local item_index = _ar:read_int()
        item_info.index = item_index
        item_info.item_id = _ar:read_int()
        item_info.check_num = _ar:read_int()
        tinsert( item_list, item_info )
        tinsert( item_index_list, item_index )
    end
   
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    
    for _, item_info in ipairs( item_list ) do
        local succ, item = check_index_item( item_info.item_id, item_info.check_num, item_info.index, player, "put_into_warehouse" )
        if not succ then return end
    end
    
    local item_index_map = {}
    for _, item_index in ipairs( item_index_list ) do
        if item_index_map[ item_index ] then 
            c_err( "[gamesvr](on_put_items_into_warehouse)player_id: %d item_index: %d is duplicated", player.player_id_, item_index )
            return
        else
            item_index_map[ item_index ] = true
        end
    end

    bag_t.put_items_into_warehouse( player, item_index_list )
    --player:do_decompose_equip( item_index_list )
end



function on_withdraw_warehouse_item( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    item_index = _ar:read_int()
    item_num = _ar:read_int()

    bag_t.withdraw_item_from_warehouse( player, item_index, item_num )
end

function on_reset_tower(_ar, _dpid, _size)
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_reset_tower()
end

function on_get_tower_first_prize( _ar, _dpid, _size )
    local tower_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:get_tower_first_prize( tower_id )
end


function on_exit_to_finish( _ar, _dpid, _sieze )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    local inst = player:get_instance()
    if not inst then return end

    inst:on_player_click_exit()
end

function on_p2p_trade_request( _ar, _dpid, _size )
    local other_player_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_request) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_request( other_player_id ) 
end

function on_p2p_trade_cancel_request(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_cancel_request) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_cancel_request( trade_sn )
end

function on_p2p_trade_respond(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local resp_code = _ar:read_byte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_respond) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_respond( trade_sn, resp_code )
end

function on_p2p_trade_cancel_trade(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_cancel_trade) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_cancel_trade( trade_sn )
end

function on_p2p_trade_add_item(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local item_index = _ar:read_int()
    local item_num = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_add_item) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_add_item( trade_sn, item_index, item_num )
end

function on_p2p_trade_remove_item(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local item_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_remove_item) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_remove_item( trade_sn, item_index )
end

function on_p2p_trade_first_confirm(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_first_confirm) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_first_confirm( trade_sn )
end

function on_p2p_trade_modify_gold(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local gold_num = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_modify_gold) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_modify_gold( trade_sn, gold_num )
end

function on_p2p_trade_deal(_ar, _dpid, _size)
    local trade_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_p2p_trade_deal) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:p2p_trade_deal( trade_sn )
end

function on_get_attend_prize( _ar, _dpid, _size )
    local prize_index = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_attend_prize) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:do_get_attend_prize( prize_index )
end

function on_get_level_prize( _ar, _dpid, _size )
    local level = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_level_prize) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:do_get_level_prize( level )
end

function on_request_active_trigger( _ar, _dpid, _size )
    local trigger_chunk_idx = _ar:read_int()
    local actor_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_request_active_trigger) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local instance = player:get_instance()
    if not instance then
        c_err( "[gamesvr](on_request_active_trigger) not not found instance by player, pid: %d", player.player_id_ )
        return
    end
    instance:on_client_active_trigger( trigger_chunk_idx, actor_id )
end

function on_daily_goals( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    local daily_goal_id = _ar:read_int()
    if not player then
        c_err( "[gamesvr](on_daily_goals) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:on_take_goal_reward( daily_goal_id )
end

function on_player_rename( _ar, _dpid, _size )
    local player_name = _ar:read_string()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    dbclient.remote_call_db( "db_login.do_player_rename_check", player.account_id_, player.player_id_, player_name )
end

function on_exchange_get_onsell_list( _ar, _dpid, _size )
    local exchange_type = _ar:read_byte()
    local page_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_exchange_get_onsell_list) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:get_gold_exchange_list( exchange_type, page_index )
end

function on_exchange_create( _ar, _dpid, _size )
    local exchange_type = _ar:read_byte()
    local amount = _ar:read_int()
    local unit_price = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_exchange_create) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:create_gold_exchange( exchange_type, amount, unit_price )
end

function on_exchange_buy( _ar, _dpid, _size )
    local server_id = _ar:read_int()
    local exchange_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gamesvr](on_exchange_buy) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:buy_gold_exchange( server_id, exchange_id )
end

function on_exchange_withdraw( _ar, _dpid, _size )
    local exchange_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_exchange_withdraw) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:withdraw_gold_exchange( exchange_id )
end

function on_exchange_retrieve( _ar, _dpid, _size )
    local exchange_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_exchange_retrieve) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:retrieve_gold_exchange( exchange_id )
end

function on_exchange_receive( _ar, _dpid, _size )
    local exchange_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_exchange_receive) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:receive_gold_exchange( exchange_id )
end

function on_instance_preload_finish( _ar, _dpid, _size )
    local instance_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        instance_mng.player_join_instance_after_preload_finish( player, instance_id )
    end
end

function on_miniserver_get_one_drop( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    local buf_len = _ar:read_int()
    local gain = drop_t.unpack_drop_level_gain( _ar )
    drop.on_miniserver_add_drop( _dpid, gain.act_inst_id_, gain.drop_level_, gain )
end 

function on_req_event_question_join( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    event_question_t.question_on_player_join( player )
end

function on_req_event_question_quit( _ar, _dpid, _size )
end

function on_req_event_question_submit( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    local param = {}
    param.player_ = player
    param.question_index_ = _ar:read_ubyte()
    param.choice_ = _ar:read_ubyte()
    param.use_pass_item_ = _ar:read_ubyte()
    param.use_double_item_ = _ar:read_ubyte()
    event_question_t.question_on_player_submit( param )
end

function question_on_player_select_double( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    local param = {}
    param.player_ = player
    param.question_index_ = _ar:read_ubyte()
    event_question_t.question_on_player_select_double( param )
end

function on_req_event_price_board_get_award( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    local inst_index = _ar:read_ubyte()
   event_price_board_t.price_board_on_player_get_award( player, inst_index )
end

function on_req_event_price_board_reset( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    event_price_board_t.price_board_try_reset( player )
end

function on_req_event_price_board_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    player:add_event_price_board_info()
end

-- 只能在miniserver新手本危情之地使用，buff_id限定101301，101302，101303，101304
function on_add_buff( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    local buff_id = _ar:read_int()

    if not g_mini_server then
        c_err( "[gamesvr](on_add_buff) PT_ADD_BUFF is miniserver only, player: %s", player.tostr_ )
    end

    local world_id = player.core_:c_get_sceneid()
    if world_id ~= 1013 then
        c_err( "[gamesvr](on_add_buff) PT_ADD_BUFF is world 1013 only, player: %s, world_id: %d", player.tostr_, world_id )
    end

    if buff_id ~= 101301 and buff_id ~= 101302 and buff_id ~= 101303 and buff_id ~= 101304 then
        c_err( "[gamesvr](on_add_buff) PT_ADD_BUFF wrong buff id, player: %s, buff_id: %d", player.tostr_, buff_id )
    end

    if not BUFFS[buff_id] then
        c_err( "[gamesvr](on_add_buff) buff cfg not found, buff_id: %d", buff_id )
        return
    end
    player:do_buff( buff_id, player.ctrl_id_ )
end

-- 只能在miniserver新手本危情之地使用，buff_id限定101301，101302，101303，101304
function on_remove_buff( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    local buff_id = _ar:read_int()

--[[    if not g_mini_server then
        c_err( "[gamesvr](on_remove_buff) PT_REMOVE_BUFF is miniserver only, player: %s", player.tostr_ )
    end--]]

    -- 只能在新手本危情之地使用
    local world_id = player.core_:c_get_sceneid()
    if world_id ~= 1013 then
        c_err( "[gamesvr](on_remove_buff) PT_REMOVE_BUFF is world 1013 only, player: %s, world_id: %d", player.tostr_, world_id )
    end

    if buff_id ~= 101301 and buff_id ~= 101302 and buff_id ~= 101303 and buff_id ~= 101304 then
        c_err( "[gamesvr](on_remove_buff) PT_REMOVE_BUFF wrong buff id, player: %s, buff_id: %d", player.tostr_, buff_id )
    end

    if not BUFFS[buff_id] then
        c_err( "[gamesvr](on_remove_buff) buff cfg not found, buff_id: %d", buff_id )
        return
    end
    player:del_buff_by_id( { [buff_id] = true } )
end

function on_player_get_leona_gift( _ar, _dpid, _size )
    local gift_cfg_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_get_leona_gift( gift_cfg_id )
    end
end

function on_recharge_award_lottery( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_lottery_recharge_award()
    end
end

function on_apply_daily_energy( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_apply_daily_energy()
    end
end

function on_runner_start( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_runner_start) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:start_runner()
end

function on_runner_quit( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_runner_quit) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:quit_runner( true )
end

function on_runner_trade_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_runner_trade_info) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local npc_id = _ar:read_int()
    player:add_runner_trade_info( npc_id )
end

function on_runner_buy_item( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_runner_buy_item) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local npc_id = _ar:read_int()
    local item_id = _ar:read_int()
    local item_num = _ar:read_int()
    local item_price = _ar:read_int()
    player:buy_runner_item( npc_id, item_id, item_num, item_price )
end

function on_runner_sell_item( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_runner_sell_item) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local npc_id = _ar:read_int()
    local item_id = _ar:read_int()
    local item_num = _ar:read_int()
    local item_price = _ar:read_int()
    player:sell_runner_item( npc_id, item_id, item_num, item_price )
end

function on_runner_receive_award( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_runner_receive_award) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:receive_runner_award()
end

function on_get_wild_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_wild_list) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local wild_type = _ar:read_int()
    local page_index = _ar:read_int()
    local zone_id = wild_mng.get_zone_id()
    bfclient.remote_call_center( "wild_mng.send_wild_list_to_client", player.server_id_, _dpid, wild_type, zone_id, page_index )
end

function on_join_wild( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_join_wild) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local wild_type = _ar:read_int()
    local wild_id = _ar:read_int()
    local trans_id = _ar:read_int()
    wild_mng.join_wild( player, wild_type, wild_id, trans_id )
end

function on_auto_join_wild( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_auto_join_wild) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local wild_type = _ar:read_int()
    local trans_id = _ar:read_int()
    wild_mng.auto_join_wild( player, wild_type, trans_id )
end

function on_pve_element( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_pve_element) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local element_type = _ar:read_byte()
    player:set_pve_element( element_type )
end

function on_pvp_element( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_pvp_element) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local element_type = _ar:read_byte()
    player:set_pvp_element( element_type )
end

function on_activate_element( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_activate_element) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local element_type = _ar:read_byte()
    player:activate_element( element_type )
end

function on_deactivate_element( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_deactivate_element) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local element_type = _ar:read_byte()
    player:deactivate_element( element_type )
end

function on_unlock_master_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_unlock_master_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local skill_id = _ar:read_int()
    player:unlock_master_skill( skill_id )
end

function on_upgrade_master_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_upgrade_master_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local skill_id = _ar:read_int()
    player:upgrade_master_skill( skill_id )
end

function on_upgrade_practice_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_upgrade_practice_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local skill_id = _ar:read_int()
    player:upgrade_practice_skill( skill_id )
end

function on_sync_practice_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_sync_practice_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local skill_id = _ar:read_int()
    local level = _ar:read_int()
    player:sync_practice_skill_level( skill_id, level )
end

function on_sync_master_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_sync_master_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local skill_id = _ar:read_int()
    local level = _ar:read_int()
    player:sync_master_skill_level( skill_id, level )
end

function on_sync_active_element( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_sync_active_element) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local element_type = _ar:read_int()
    player:sync_active_element( element_type )
end

function on_sync_transform_model( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_sync_transform_model) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local model_index = _ar:read_int()
    player:miniserver_sync_transform_model( model_index )
end

function on_sync_transform_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_sync_transform_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local skill_id = _ar:read_int()
    player:miniserver_sync_transform_skill( skill_id ) 
end

function on_sync_transform_dynamic_skill( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_sync_transform_dynamic_skill) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local model_index = _ar:read_int()
    local skill_index = _ar:read_int()
    local book_id = _ar:read_int()
    local level = _ar:read_int()
    player:miniserver_sync_transform_dynamic_skill( model_index, skill_index, book_id, level )
end


function on_holiday_open_box( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_holiday_open_box) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:open_box()
end

function on_get_rank_list( _ar, _dpid, _size )
   rank_net.on_get_rank_list(_ar, _dpid, _size)
end

function on_set_recharge_reward_consume_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_set_recharge_reward_consume_list) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local consume_index = _ar:read_int()
    player:on_player_earl_reward_consume(consume_index)
end

function on_set_title( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_set_title) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local title_id = _ar:read_ushort()
    player:on_set_title(title_id)
end



function on_get_recharge_reward_diamond(_ar, _dpid, _size)
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_recharge_reward_diamond) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local reward_index = _ar:read_int()
    player:on_get_recharge_reward_diamond(reward_index)
end

function on_get_truncheon(_ar, _dpid, _size)
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_truncheon) can not found player by dpid: 0x%X", _dpid )
        return
    end

    player:on_get_truncheon()
end

function on_get_first_recharge_award(_ar, _dpid, _size)
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_get_first_recharge_award) can not found player by dpid: 0x%X", _dpid )
        return
    end

    local index = _ar:read_int()
    player:on_get_first_recharge_award(index)
end

function on_guild_create(_ar, _dpid, _size)
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_create) can not found player by dpid: 0x%X", _dpid )
        return
    end

    local guild_name = _ar:read_string()
    local guild_desc = _ar:read_string()
    player:create_guild( guild_name, guild_desc )
end

function on_guild_quit( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_create) can not found player by dpid: 0x%X", _dpid )
        return
    end

    player:quit_guild()
end

function on_guild_change_desc( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_create) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local guild_desc = _ar:read_string()
    local guild_announce = _ar:read_string()
    player:change_desc( guild_desc, guild_announce)
end

function on_guild_handle_apply( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_handle_apply) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local is_approved_num = _ar:read_ubyte()
    local is_approved = is_approved_num == 1
    local apply_server_id = _ar:read_int()
    local apply_player_id = _ar:read_int()
    player:handle_apply_guild( is_approved, { apply_server_id, apply_player_id })
end

function on_guild_search_by_name( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_handle_apply) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local guild_server_id = _ar:read_int()
    local guild_name = _ar:read_string()
    player:on_guild_search_by_name( guild_server_id, guild_name )
end

function on_guild_apply( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_handle_apply) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local guild_server_id = _ar:read_int()
    local guild_id = _ar:read_int()
    player:apply_guild( guild_server_id, guild_id )
end

function on_guild_handle_invite( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_handle_invite) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local server_id = _ar:read_int()
    local guild_id = _ar:read_int()
    local is_accept = _ar:read_byte()
    if is_accept == 1 then
        player:accept_invitation( server_id, guild_id )
    else
        player:reject_invitation( server_id, guild_id )
    end
end

function on_guild_kick( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_kick) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    player:kick_from_guild( {server_id, player_id} )
end

function on_guild_set_role( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_set_role) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    local role = _ar:read_byte()
    player:set_guild_member_role( {server_id, player_id}, role )
end

function on_guild_cancel_apply( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_cancel_apply) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local server_id = _ar:read_int()
    local guild_id = _ar:read_int()
    player:cancel_apply( server_id, guild_id )
end

function on_guild_get_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_get_list) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local index = _ar:read_short()
    player:get_guild_list( index )
end

function on_guild_learn_profession( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_learn_profession) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local prof_id = _ar:read_int()
    player:learn_profession( prof_id )
end    

function on_guild_drop_profession( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_drop_profession) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:drop_profession()
end    

function on_guild_apply_profession( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_apply_profession) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:apply_profession()
end    

function on_guild_handle_apply_profession( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_handle_apply_profession) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local is_approved = _ar:read_boolean()
    local prof_id = _ar:read_int()
    local applier_server_id = _ar:read_int()
    local applier_player_id = _ar:read_int()
    player:handle_apply_profession( is_approved, prof_id, {applier_server_id, applier_player_id} )
end    

function on_guild_kick_profession( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_handle_apply_profession) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local prof_id = _ar:read_int()
    local applier_server_id = _ar:read_int()
    local applier_player_id = _ar:read_int()
    player:kick_profession( prof_id, {applier_server_id, applier_player_id} )
end    

function on_guild_prof_reject_apply( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_prof_reject_apply) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local applier_server_id = _ar:read_int()
    local applier_player_id = _ar:read_int()
    player:reject_goods_applier( {applier_server_id, applier_player_id} )
end    

function on_guild_prof_apply_goods( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_prof_apply_goods) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local prof_id = _ar:read_int()
    local welfare_id = _ar:read_int()
    local goods_item_id = _ar:read_int()
    player:apply_prof_goods( prof_id, welfare_id, goods_item_id )
end    

function on_guild_prof_delivery_goods( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_prof_delivery_goods) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local welfare_id = _ar:read_int()
    local goods_item_id = _ar:read_int()
    local applier_server_id = _ar:read_int()
    local applier_player_id = _ar:read_int()
    player:delivery_goods( welfare_id, goods_item_id, {applier_server_id, applier_player_id} )
end

function on_guild_prof_composite_goods( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_prof_composite_goods) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local goods_item_id = _ar:read_int()
    local num = _ar:read_int()
    player:composite_prof_goods( goods_item_id, num )
end

function on_guild_enter_guild_scene( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_enter_guild_scene) can not found player by dpid: 0x%X", _dpid )
        return
    end
    player:on_enter_guild_scene_request()
end

function on_guild_prof_activate_goods( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_prof_activate_goods) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local welfare_id = _ar:read_int()
    local goods_item_id = _ar:read_int()
    player:on_guild_prof_activate_goods( welfare_id, goods_item_id )
end

function on_guild_prof_cancel_goods_effect( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_prof_activate_goods) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local welfare_id = _ar:read_int()
    local goods_item_id = _ar:read_int()
    player:on_guild_prof_cancel_goods_effect( welfare_id, goods_item_id )
end

function on_guild_donate_money( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_donate_money) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local money = _ar:read_int()
    player:on_guild_donate_money( money )
end

function on_guild_donate_diamond( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_donate_diamond) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local diamond = _ar:read_int()
    player:on_guild_donate_diamond( diamond )
end

function on_donate_set_activate_id( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_donate_diamond) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local activate_id = _ar:read_byte()
    player:on_guild_donate_set_activate_id( activate_id )
end

function on_guild_rename( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_guild_donate_diamond) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local name = _ar:read_string()
    player:rename_guild( name )
end

function on_player_enroll_weekend_pvp( _ar, _dpid, _size )
    local select_model_num = _ar:read_ubyte()
    local select_model_list = {}
    for i = 1, select_model_num do
        local model_index = _ar:read_ubyte()
        tinsert( select_model_list, model_index )
    end
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_weekend_pvp.on_player_enroll_weekend_pvp( player, select_model_list )
    end
end

function on_request_join_weekend_pvp_battle( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_weekend_pvp.on_player_request_join_battle( player )
    end
end

function on_open_weekly_task( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:open_weekly_task()
    end
end

function on_finish_weekly_task( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:finish_weekly_task()
    end
end

function on_get_task_question( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        task_misc.get_question( player )
    end
end

function on_answer_task_question( _ar, _dpid, _size )
    local question_id = _ar:read_int()
    local answer = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        task_misc.answer_question( player, question_id, answer )
    end
end

function on_enter_team_inst( _ar, _dpid, _size ) 
    local inst_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        team_inst_gs.leader_start_inst( player, inst_id ) 
    end
end

function on_truncheon_refine( _ar, _dpid, _size, _is_by_item )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local is_by_item = _is_by_item or _ar:read_int()
    local ret, add_exp, is_crit, is_up, rand  = player:do_truncheon_refine( is_by_item )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("(on_truncheon_refine) player_id:%d ret:%d is_by_item:%d, rand:%d",
        player.player_id_, ret, is_by_item, rand or -1)
    player:add_truncheon_refine( ret, add_exp, is_crit, is_up )
end

function on_truncheon_reborn( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local slot_index = _ar:read_int()
    local is_by_item = _ar:read_int()
    local ret = player:do_truncheon_reborn( slot_index, is_by_item )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("(on_truncheon_reborn) player_id:%d ret:%d is_by_item:%d ",
        player.player_id_, ret, is_by_item )
    player:add_truncheon_reborn( ret, slot_index )
end

function on_truncheon_reborn_confirm( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local is_accept = _ar:read_int()
    local ret = player:do_truncheon_reborn_confirm( is_accept )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("(on_truncheon_reborn_confirm) player_id:%d ret:%d is_accept:%d",
        player.player_id_, ret, is_accept )
    player:add_truncheon_reborn_confirm( ret, is_accept )
end

function on_truncheon_reborn_exchange( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local exchange_cnt = _ar:read_int()
    local ret = player:do_truncheon_reborn_exchange( exchange_cnt )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("(do_truncheon_reborn_exchange) player_id:%d, ret:%d, exchange_cnt:%d",
        player.player_id_, ret, exchange_cnt )
    player:add_truncheon_reborn_exchange( ret, exchange_cnt )
end


function on_truncheon_lettering( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local slot_index = _ar:read_int()
    local is_by_item = _ar:read_int()
    local ret = player:do_truncheon_lettering( slot_index, is_by_item )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("(on_truncheon_lettering) player_id:%d ret:%d is_by_item:%d ",
        player.player_id_, ret, is_by_item )
    player:add_truncheon_lettering( ret, slot_index )
end

function on_truncheon_lettering_confirm( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local is_accept = _ar:read_int()
    local ret = player:do_truncheon_lettering_confirm( is_accept )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("(on_truncheon_lettering_confirm) player_id:%d ret:%d is_accept:%d",
        player.player_id_, ret, is_accept )
    player:add_truncheon_lettering_confirm( ret, is_accept )
end

function on_five_pvp_invite_friend( _ar, _dpid, _size )
    local sid = _ar:read_int()
    local pid = _ar:read_int()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        five_pvp_match_gs.player_invite_friend( player, sid, pid )
    end
end

function on_five_pvp_invite_accept( _ar, _dpid, _size )
    local sid = _ar:read_int()
    local pid = _ar:read_int()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        five_pvp_match_gs.player_accept_invite( player, sid, pid )
    end
end

function on_five_pvp_invite_refuse( _ar, _dpid, _size)
    local sid = _ar:read_int()
    local pid = _ar:read_int()
    
    local player = player_mng.get_player_by_dpid( _dpid )   
    if player then
        five_pvp_match_gs.player_refuse_invite( player, sid, pid )
    end
end

function on_five_pvp_quit_team( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )   
    if player then
        five_pvp_match_gs.player_quit_team( player )
    end
end

function on_five_pvp_kick_teammate( _ar, _dpid, _size ) 
    local kick_sid = _ar:read_int()
    local kick_pid = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        five_pvp_match_gs.captain_kick_member( player, kick_sid, kick_pid ) 
    end
end

function on_five_pvp_request_team_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        five_pvp_match_gs.player_update_team_info( player ) 
    end
end

function on_five_pvp_clear_reminder( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        five_pvp_match_gs.remove_waiting_join_player( player.player_id_ )
    end
end

function on_five_pvp_join_team_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        five_pvp_match_gs.call_bf_join_team_match( player )
    end
end

function on_leona_present( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_apply_leona_present()
    end
end

function on_request_exchange_equip( _ar, _dpid, _size )
    local exchange_cfg_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_request_exchange_equip( exchange_cfg_id )
    end
end

function on_request_equip_exchange_shop_items( _ar, _dpid, _size )
    local type_num = _ar:read_int()
    local type_list = {}
    for i = 1, type_num do
        tinsert( type_list, _ar:read_ubyte() )
    end
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_client_request_equip_items( type_list )
    end
end

function on_member_confirm_weekend_pvp_enroll( _ar, _dpid, _size )
    local bf_id = _ar:read_int()
    local initial_server_id = _ar:read_int()
    local team_id = _ar:read_int()
    local select_model_num = _ar:read_ubyte()
    local select_model_list = {}
    for i = 1, select_model_num do
        local model_index = _ar:read_ubyte()
        tinsert( select_model_list, model_index )
    end

    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_weekend_pvp.on_team_member_confirm_enroll( bf_id, initial_server_id, player.player_id_, team_id, select_model_list )
    end
end

function on_ack_enter_millionaire_inst( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_ack_enter_millionaire_inst) can not found player by dpid: 0x%X", _dpid )
        return
    end
    millionaire_pvp.leader_click_start_millionaire_pvp(player)
end

function on_req_rejoin_millionaire( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    millionaire_pvp.millionaire_send_player_to_bf_again( player )
end

function on_req_millionaire_status( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    millionaire_pvp.millionaire_send_player_rejoin_status( player )
end

function on_ack_millionaire_player_pos( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    millionaire_pvp.client_ack_gs_player_pos( player, server_id, player_id )
end

function on_team_inst_rejoin( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then 
        team_inst_gs.on_player_rejoin_team_inst( player )
    end
end

function on_stronger_gs_grade( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end 

    player:add_gear_score_grade( player.gear_score_percentage_ )
end

function on_welfare_buy( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        return
    end
    local buy_id = _ar:read_int()
    local ret = player:do_welfare_buy( buy_id )
    local logfunc = (ret>= 0) and c_log or c_err
    logfunc("[gamesvr](on_welfare_buy) .. player_id: %d , ret: %d", player.player_id_, ret )
    player:add_welfare_buy_ret(buy_id,ret)
end

function on_recharge_remind_open( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then 
        player:on_recharge_remind_open()
    end
end

function on_buy_gold_ui_opened( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        return
    end
    player:do_buy_gold_ui_opened()
end

function on_buy_gold( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then 
        return
    end
    local diamond_type = _ar:read_int()
    local ret, is_crit, gold_num, buy_gold_cnt = player:do_buy_gold(diamond_type)
    player:add_buy_gold_ret(ret, diamond_type, is_crit, gold_num )
    local log_func = (ret == 0) and c_log or c_err
    log_func("[player_t]on_buy_gold player_id:%d, ret:%d, is_crit:%d, gold_num:%d, buy_gold_cnt:%d, diamond_type:%d", player.player_id_, ret or -1, is_crit or -1, gold_num or -1, buy_gold_cnt or -1, diamond_type)
end


function on_fetch_player_attr( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then 
        player:add_report_player_attr()
    end
end

function on_putaway_item( _ar, _dpid, _size )
    trade_net.on_putaway_item(_ar, _dpid, _size)
end

function on_buy_trade_item( _ar, _dpid, _size )
    trade_net.on_buy_trade_item(_ar, _dpid, _size)
end

function on_sold_out_item( _ar, _dpid, _size )
    trade_net.on_sold_out_item( _ar, _dpid, _size )
end

function on_query_trade_item_list( _ar, _dpid, _size )
    trade_net.on_query_trade_item_list( _ar, _dpid, _size )
end

function on_modify_trade_price( _ar, _dpid, _size )
    trade_net.on_modify_trade_price( _ar, _dpid, _size )
end

function on_add_favorite_item( _ar, _dpid, _size )
    trade_net.on_add_favorite_item( _ar, _dpid, _size )
end

function on_remove_favorite_item( _ar, _dpid, _size )
    trade_net.on_remove_favorite_item( _ar, _dpid, _size )
end

function on_get_favorite_item_list( _ar, _dpid, _size )
    trade_net.on_get_favorite_item_list( _ar, _dpid, _size )
end

function on_appoint_wittnight( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:appoint_wittrya_night()
    end
end

function on_elam_military_supplies_draw_free( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_elam_military_supplies_draw_free()
    end
end

function on_elam_military_supplies_draw_once( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_elam_military_supplies_draw_once()
    end
end

function on_elam_military_supplies_draw_ten( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_elam_military_supplies_draw_ten()
    end
end

function on_join_one_gold_lucky_draw( _ar, _dpid, _size )
    local round_id = _ar:read_int()
    local batches = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:join_one_gold_lucky_draw( round_id, batches )
    end
end

function on_open_one_gold_lucky_draw_ui( _ar, _dpid, _size )
    local is_open = _ar:read_boolean()
    local max_completed_time = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_open_one_gold_lucky_draw_ui( is_open, max_completed_time )
    end
end

function on_one_gold_lucky_draw_exchange_bonus( _ar, _dpid, _size )
    local goods_id = _ar:read_int()
    local batches = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_one_gold_lucky_draw_exchange_bonus( goods_id, batches )
    end
end

function on_open_chat_gift_box( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_open_chat_gift_box()
    end
end

function on_get_christmas_sell_award( _ar, _dpid, _size )
    local award_id = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_get_christmas_sell_award( award_id )
    end
end

function on_open_christmas_recharge_lucky_draw_box( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_open_christmas_recharge_lucky_draw_box()
    end
end

function on_upgrade_wing( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_upgrade_wing()
    end
end

function on_wing_core_upgrade( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
		local core_id = _ar:read_int()
        player:on_upgrade_wing_core_star(core_id)
    end
end

function on_wing_core_equip( _ar, _dpid, _size )   --PT_WING_CORE_EQUIP
	local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
		local core_id = _ar:read_int()
        player:on_wing_core_equip(core_id)
    end
end

function on_wing_skill_unlock( _ar, _dpid, _size )   -- 技能解锁
	local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
		local skill_id = _ar:read_int()
        player:on_wing_skill_unlock(skill_id)
    end
end

function on_wing_skill_upgrade( _ar, _dpid, _size )   -- 技能解锁
	local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
		local skill_id = _ar:read_int()
        player:on_wing_skill_upgrade(skill_id)
    end
end

function on_query_refine_attr( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid ) 	
    if player then 
		local item_index = ar:read_int()
		local item_id = _ar:read_int()		
		local item_num = ar:read_int()
		local succ, item = check_index_item( item_id, item_num, item_index, player, "on_query_refine_attr" )
		if not succ then
			return
		end
        player:on_query_refine_attr(item)
    end
end

function on_add_competitive_halo( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_add_competitive_halo()
    end
end



function on_robot_add_item(_ar, _dpid, _size)
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:on_robot_add_item()
    end
end

--[[
function on_guild_daily_pvp_enroll( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        guild_daily_pvp.on_guild_master_request_enroll( player )
    end
end
--]]

function on_guild_daily_pvp_join_battle( _ar, _dpid, _size )
    local request_code = _ar:read_ubyte()
    local team_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        guild_daily_pvp.on_player_request_join_battle( player, request_code, team_id ) 
    end
end

function on_guild_daily_pvp_create_team( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return
    end
    local ret = guild_daily_pvp.on_create_team( player )
    local log_func = ret >= 0 and c_log or c_err
    log_func("on_guild_daily_pvp_create_team player_id:%d, ret:%d", player.player_id_, ret)
    player:add_guild_daily_pvp_create_team( ret )
end

function on_guild_daily_pvp_delete_team( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return
    end
    local ret = guild_daily_pvp.on_delete_team( player )
    local log_func = ret >= 0 and c_log or c_err
    log_func("on_guild_daily_pvp_delete_team player_id:%d, ret:%d", player.player_id_, ret)
    player:add_guild_daily_pvp_delete_team( ret )
end

function on_guild_daily_pvp_join_team( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return
    end
    local ret = guild_daily_pvp.on_join_team( player )
    local log_func = ret >= 0 and c_log or c_err
    log_func("on_guild_daily_pvp_join_team player_id:%d, ret:%d", player.player_id_, ret)
    player:add_guild_daily_pvp_join_team( ret )
end

function on_guild_daily_pvp_quit_team( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return
    end
    local ret = guild_daily_pvp.on_quit_team( player )
    local log_func = ret >= 0 and c_log or c_err
    log_func("on_guild_daily_pvp_quit_team player_id:%d, ret:%d", player.player_id_, ret)
    player:add_guild_daily_pvp_quit_team( ret )
end

function on_guild_daily_pvp_kick_team( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return
    end
    local kick_id = _ar:read_int()
    local ret = guild_daily_pvp.on_kick_team( player, kick_id )
    local log_func = ret >= 0 and c_log or c_err
    log_func("on_guild_daily_pvp_kick_team player_id:%d, kick_id:%d, ret:%d", player.player_id_, kick_id, ret)
    player:add_guild_daily_pvp_kick_team( ret )
end


function on_auction_item( _ar, _dpid, _size )
    auction_net.on_auction_item( _ar, _dpid, _size )
end

function on_cancel_auction_item( _ar, _dpid, _size )
    auction_net.on_cancel_auction_item( _ar, _dpid, _size )
end
function on_query_auction_item( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
          return
    end

    local curtime = os.time()
    player.query_auction_count_ = player.query_auction_count_ or 0     
    if not player.prev_query_auction_time_ then 
        player.prev_query_auction_time_ = curtime
    end 
    local prev_time = player.prev_query_auction_time_ or curtime
    if ( curtime - prev_time ) >  2  then 
          if player.query_auction_count_ > 50 then
                c_err("on_query_auction_item  too many times ! player_id: [%d] " , player.player_id_ )
                player_mng.kick_player_offline( player.player_id_ , player.server_id_ )
                return
          end
          player.query_auction_count_ = 0 
          player.prev_query_auction_time_ = os.time()
    end 
  
    player.query_auction_count_ = player.query_auction_count_ + 1 
    
    auction_net.on_query_auction_item( _ar, _dpid, _size )
end

function on_bid_auction_item( _ar, _dpid, _size )
    auction_net.on_bid_auction_item( _ar, _dpid, _size )
end

function on_get_bid_item( _ar, _dpid, _size )
    auction_net.on_get_bid_item( _ar, _dpid, _size )
end

function on_get_item_auction_statistic( _ar, _dpid, _size )
    auction_net.on_get_item_auction_statistic( _ar, _dpid, _size )
end

function on_guild_pve_start( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local player_guild = player.guild_
    local player_guild_id = player_guild.guild_id_

    if not guild_mng.is_unlock_module(player_guild_id  , guild_mng.GUILD_MODULE_MONSTER_SIEGE ) then 
        return
    end 

    local diff_idx = _ar:read_int()
    local ret = player:on_guild_pve_start( diff_idx )
    if not ret then
        return
    end
    local logfunc = ( ret >=0 ) and c_log or c_err
    logfunc("on_guild_pve_start player_id:%d, ret:%d", player.player_id_, ret )
    player:add_guild_pve_start_ret( ret )
end


function on_guild_pve_notify( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local notify_type = _ar:read_int()
    local ret = player:on_guild_pve_notify( notify_type )
    if not ret then
        return
    end
    local logfunc = ( ret >=0 ) and c_log or c_err
    logfunc("on_guild_pve_notify player_id:%d, ret:%d, notify_type:%d", player.player_id_, ret, notify_type )
    player:add_guild_pve_notify( ret, notify_type )
end

function on_guild_weekend_enroll(  _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        guild_weekend_enroll.start_enroll( player ) 
    end
end

function on_guild_weekend_enter( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        guild_weekend_mng_gs.check_enter_guild_weekend_battle( player ) 
    end
end

function on_open_guild_boutique( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:open_guild_boutique()
    end
end

function on_buy_boutique_item( _ar, _dpid, _size ) 
    local item_idx = _ar:read_int()
    --local open_time =  _ar:read_int() 
    local buy_count = _ar:read_int()    --购买数量  -- ???
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:buy_boutique_item(item_idx, buy_count)
    end
end

function on_ask_for_pay_boutique_item( _ar, _dpid, _size ) 
    local item_idx = _ar:read_int()
    --local open_time =  _ar:read_int() 
    local item_count  = _ar:read_int()  -- ???
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:ask_for_pay_boutique_item(item_idx, item_count)
    end
end

function on_pay_boutique_item_for_others( _ar, _dpid, _size ) 
    local item_idx = _ar:read_int()
    local player_id = _ar:read_int()
    local open_time =  _ar:read_int() 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:pay_boutique_item_for_others(item_idx, player_id, open_time)
    end
end

function on_god_divination_draw_runes( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:draw_god_divination_runes()
    end
end

function on_god_divination_buy_energy( _ar, _dpid, _size )
    local times = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then 
        player:buy_god_divination_energy(times)
    end
end

function on_get_level_reward( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local reward_index = _ar:read_int()
    local ret, item_list = player:do_get_level_reward( reward_index )
    local logfunc = (ret == 0) and c_log or c_err
    logfunc("on_get_level_reward, player_id:%d, ret:%d, reward_index:%d", player.player_id_, ret, reward_index) 
    player:send_get_level_reward_ret( reward_index, ret )
    if item_list then
        player:add_get_item_tip_board_by_item_list( item_list )
    end
end

function on_pet_unlock( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local pet_id = _ar:read_int()
    local ret = player:do_pet_unlock( pet_id, false )
    local logfunc = (ret == 0) and c_log or c_err
    logfunc("on_pet_unlock, player_id:%d, ret:%d, pet_id:%d", player.player_id_, ret, pet_id) 
    player:send_pet_unlock( ret, pet_id )
end

function on_pet_upgrade( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local pet_id = _ar:read_int()
    local ret, level = player:do_pet_upgrade( pet_id )
    local logfunc = (ret == 0) and c_log or c_err
    logfunc("on_pet_upgrade, player_id:%d, ret:%d, pet_id:%d, level:%d", player.player_id_, ret, pet_id, level or -1) 
    player:send_pet_upgrade( ret, pet_id, level )	
end

function on_pet_active( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local pet_id = _ar:read_int()
	local avatar_id = _ar:read_int()
    local ret, ret_avatar_id = player:do_pet_active( pet_id, avatar_id )
    local logfunc = (ret == 0) and c_log or c_err
    logfunc("on_pet_active, player_id:%d, ret:%d, pet_id:%d, avatar_id:%d", player.player_id_, ret, pet_id, ret_avatar_id or avatar_id) 
    player:send_pet_active( ret, pet_id, ret_avatar_id or avatar_id )
end

-- pet_rename改的实际上是pet avatar的名字，其实没有pet_name这个概念（策划要求）
function on_pet_rename( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return
    end
    local pet_id = _ar:read_int()
	local avatar_id = _ar:read_int()
    local avatar_name = _ar:read_string()
    local ret, pet_ctrl_id = player:do_pet_avatar_rename( pet_id, avatar_id, avatar_name )
    local logfunc = (ret == 0) and c_log or c_err
    logfunc("on_pet_rename, player_id:%d, ret:%d, pet_id:%d, avatar_id:%d, avatar_name:%s, pet_ctrl_id", player.player_id_, ret, pet_id, avatar_id, avatar_name, pet_ctrl_id or 0) 
    player:send_pet_rename( ret, pet_id, avatar_id, avatar_name, pet_ctrl_id )
end

function on_get_holiday_signin_award( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end

    local day_offset = _ar:read_uint()
    player:on_get_holiday_signin_award( day_offset )
end

function on_get_monthly_signin_award( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end

    local get_type = _ar:read_int()
    player:on_get_monthly_signin_award( get_type )
end

function on_get_daily_online_award( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end

    local award_id = _ar:read_int()
    player:on_get_daily_online_award( award_id )
end

function on_get_cycled_recharge_award( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end

    local award_id = _ar:read_int()
    player:on_get_cycled_recharge_award( award_id )
end

function on_buy_growth_fund( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end

    player:on_buy_growth_fund( )
end

function on_get_growth_fund( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end

    local award_id = _ar:read_int()
    player:on_get_growth_fund( award_id )
end

function on_buy_mall_product( _ar, _dpid, _size ) 
    local index = _ar:read_int() 
    local c = _ar:read_int() 
    local real_index = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end 
    player:buy_mall_product( index, c, real_index )  
end

function on_req_create_ladder_team( _ar, _dpid, _size )
    local name = _ar:read_string()
    local desc = _ar:read_string()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_player_request_create_team( player, name, desc )
    end
end

function on_apply_join_ladder_team( _ar, _dpid, _size )
    local server_id = _ar:read_int()
    local team_id = _ar:read_int()
    local is_apply = _ar:read_ubyte()
    local is_recruit = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then
        return 
    end

    if is_apply > 0 then
        event_team_ladder.on_player_apply_join_team( player, server_id, team_id, is_recruit > 0 )
    else
        event_team_ladder.on_player_cancel_apply( player, server_id, team_id )
    end
end

function on_req_ladder_team_list( _ar, _dpid, _size )
    local page_idx = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_player_request_ladder_team_list( player, page_idx )
    end
end

function on_req_ladder_team_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        player:get_ladder_team_info()
    end
end

function on_kick_ladder_team_member( _ar, _dpid, _size ) 
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_captain_kick_member( player, server_id, player_id )
    end
end

function on_req_ladder_team_apply_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        player:get_ladder_team_apply_list()
    end
end

function on_handle_ladder_team_join_apply( _ar, _dpid, _size )  
    local is_approve_val = _ar:read_ubyte()
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        local is_approve = is_approve_val > 0
        event_team_ladder.on_captain_handle_join_apply( player, is_approve, server_id, player_id )
    end
end

function on_req_quit_ladder_team( _ar, _dpid, _size )  
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_player_request_quit_team( player )
    end
end

function on_del_not_full_ladder_team_notify( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_del_follow_record( player.player_id_ )
    end
end

function on_change_ladder_team_text( _ar, _dpid, _size )
    local text_type = _ar:read_ubyte()
    local text = _ar:read_string()   
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_captain_change_team_text( player, text_type, text )
    end
end

function on_get_login_award(  _ar, _dpid, _size )
    local day_idx = _ar:read_int()
    local goal_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        player:get_login_award(day_idx, goal_id)
    end
end

function on_get_login_task_award(  _ar, _dpid, _size )
    local day_idx = _ar:read_int()
    local task_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        player:get_login_task_award(day_idx, task_id)
    end
end

function on_get_recharge_award(  _ar, _dpid, _size )
    local day_idx = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        player:get_recharge_award(day_idx)
    end
end

function on_buy_half_price_item(  _ar, _dpid, _size )
    local day_idx = _ar:read_int()
    local item_idx = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        player:buy_half_price_item(day_idx, item_idx)
    end
end

function on_req_join_event_team_ladder_match( _ar, _dpid, _size )
    local is_join = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        if is_join > 0 then
            event_team_ladder.on_player_request_join_match( player )
        else
            event_team_ladder.on_player_cancel_join_match( player )
        end
    end
end

function on_confirm_event_team_ladder_match( _ar, _dpid, _size )
    local is_approve = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_member_confirm_match( player, is_approve > 0 )
    end
end

function on_req_join_event_team_ladder_battle( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_player_request_join_battle( player )
    end
end

function on_invite_friend_join_ladder_team( _ar, _dpid, _size )
    local friend_sid = _ar:read_int()
    local friend_pid = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_captain_invite_friend_join_team( player, friend_sid, friend_pid )
    end
end

function on_handle_ladder_team_invite( _ar, _dpid, _size )
    local team_sid = _ar:read_int()
    local team_pid = _ar:read_int()
    local is_approve = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_player_handle_invite( player, team_sid, team_pid, is_approve > 0 )
    end
end

function on_search_not_full_team( _ar, _dpid, _size )
    local team_name = _ar:read_string()
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if player then
        event_team_ladder.on_player_search_not_full_team( player, team_name )
    end
end

function on_req_ladder_team_recruit( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )         
    if player then                                                
        event_team_ladder.on_player_request_team_recruit( player )
    end                                                           
end 

function on_req_team_ladder_rank( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )         
    if player then                                                
        event_team_ladder.on_player_request_team_rank( player )
    end                                                           
end 

function on_get_celebration_award( _ar, _dpid, _size )
    local get_type = _ar:read_int()
    local reward_index = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:get_celebration_reward( get_type, reward_index )
end

function on_cele_lottery_one( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:get_celebration_lottery_one()
end

function on_cele_lottery_ten( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:get_celebration_lottery_ten()
end

function on_cele_invest( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:do_invest()
end

function on_check_lottery_update( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:check_lottery_update()
end

function on_buy_lottery_mall_product( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    local product_id = _ar:read_int()
    local count = _ar:read_int()
    player:buy_lottery_mall_product( product_id, count )
end

function on_req_player_lucky_rate_data( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    local is_self = _ar:read_int()
    if is_self == 1 then
        lucky_rate.get_my_self_lucky_rate( player )
    else
        lucky_rate.get_ramdon_player_lucky_rate( player )
    end
end

function on_transfer_elam_money_2_voucher( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end

    player:transfer_elam_money_2_voucher()
end

function on_transfer_voucher_2_diamond( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:transfer_voucher_2_diamond( )
end

function on_sanding( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    local sanding_type = _ar:read_int()
    local ret, end_time = player:do_sanding( sanding_type )
    local log_func = (ret >= 0) and c_log or c_err
    log_func("[gamesvr](on_sanding) player_id:%d, sanding_type:%d, ret:%d, end_time:%d", player.player_id_, sanding_type, ret, end_time or -1 )
   player:send_sanding_ret( sanding_type, ret, end_time or -1) 
end

function on_open_officer_box( _ar, _dpid, _size )
    local box_level = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then return end
    player:on_open_officer_box( box_level )
end


function on_ask_syn_mall_product( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return 
    end 
    player:on_syn_all_product()
end

function on_quick_match_inst( _ar, _dpid, _size ) 
    local multi_id = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return 
    end 
    
    team_inst_gs.on_player_quick_match(player, multi_id)
end

function on_confirm_quick_match( _ar, _dpid, _size ) 
    local is_confirm = _ar:read_byte() == 1 and true or false
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then 
        return 
    end 
    
    team_inst_gs.on_player_confirm_quick_match(player, is_confirm)
end

function on_cancel_quick_match( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid ) 

    if not player then 
        return 
    end 
    
    team_inst_gs.on_cancel_quick_match(player)
end

function on_commit_mipush_regid( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local regid = _ar:read_string()
    
    player:update_mipush_regid( regid )
end

function on_get_tower_rank_list( _ar, _dpid, _size )
    local client_rank_list_version = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    tower_rank_mng.send_tower_rank_list( player, client_rank_list_version )
end

function on_get_palace_rank_list( _ar, _dpid, _size )
    local client_rank_list_version = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    palace_rank_mng.send_palace_rank_list( player, client_rank_list_version )
end

function on_hall_try_refresh( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end
    local count = _ar:read_int()
    if count > 0 then
        for i = 1, count do
            local hall_type = _ar:read_int()
            local hall_ver = _ar:read_int()
            local hall_sn = _ar:read_int()
            hall_mng.on_try_refresh( player, hall_type, hall_ver, hall_sn )
        end
    end
end

function on_hall_upvote( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end
    local hall_type = _ar:read_int()
    local hall_index = _ar:read_int()
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    local ret = player:do_hall_upvote( hall_type, hall_index, server_id, player_id )
    local is_succ = ret == 0
    local log_func = is_succ and c_trace or c_err
    log_func("[gamesvr](on_hall_upvote) self_player_id:%d, hall_type:%d, hall_index:%d, server_id:%d, player_id:%d, ret:%d", player.player_id_, hall_type, hall_index, server_id, player_id, ret )
    if is_succ then
        return
    end
    player:add_hall_upvote_ret( ret, hall_type, hall_index, server_id, player_id, nil)
end

function on_hall_upvote_data( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end
    local hall_type = _ar:read_int()
    local hall_index = _ar:read_int()
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    local ret = hall_mng.do_get_hall_upvote_data( hall_type, hall_index, server_id, player_id, player )
    local is_succ = ( ret == 0 )
    local log_func = is_succ and c_trace or c_err
    log_func("[gamesvr](on_hall_upvote_data) self_player_id:%d, hall_type:%d, hall_index:%d, server_id:%d, player_id:%d, ret:%d", player.player_id_, hall_type, hall_index, server_id, player_id, ret )
    if is_succ then
        return
    end
    
    player:add_hall_upvote_data( ret, hall_type, hall_index, server_id, player_id, nil)
end

function on_hall_set_greeting_and_dialogue( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end
    local hall_type = _ar:read_int()
    local hall_index = _ar:read_int()
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    local greeting = _ar:read_string()
    local dialogue = _ar:read_string()
    local ret = player:do_hall_set_greeting_and_dialogue( hall_type, hall_index, server_id, player_id, greeting, dialogue )
    local log_func = (ret == 0) and c_trace or c_err
    log_func("[gamesvr](on_hall_set_greeting_and_dialogue) self_player_id:%d, hall_type:%d, hall_index:%d, server_id:%d, player_id:%d, greeting:%s, dialogue:%s, ret:%d", player.player_id_, hall_type, hall_index, server_id, player_id, greeting, dialogue, ret )
    player:add_hall_set_greeting_and_dialogue_ret( ret, hall_type, hall_index, server_id, player_id, greeting, dialogue )

end

function on_hall_upvote_rank_data( _ar, _dpid, _size ) 
    local player = player_mng.get_player_by_dpid( _dpid ) 
    if not player then return end
    player:add_hall_upvote_rank_data()
end

function on_request_team_champion_enroll( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_enroll_match( player )
    end
end

function on_request_team_champion_qualification_list( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_qualification_list( player )
    end
end

function on_request_team_champion_match_record( _ar, _dpid, _size )
    local round = _ar:read_ubyte()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_match_record( player, round )
    end
end

function on_team_champion_delete_follow_match_player( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.delete_follow_match_player( player )
    end
end

function on_team_champion_request_join_battle( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_join_battle( player )
    end
end

function on_team_champion_request_watch_match( _ar, _dpid, _size )
    local round = _ar:read_ubyte()
    local match_rec_id = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_watch_match( player, round, match_rec_id )
    end
end

function on_team_champion_request_match_rank(  _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_match_rank( player )
    end
end

function on_query_rebate_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )

    if not player then
        return
    end

    rebate_mng.query_rebate( player )
end

function on_confirm_rebate( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )

    if not player then
        return
    end

    rebate_mng.confirm_rebate( player )
end

function on_get_chapter_award( _ar, _dpid, _size )
    local task_id = _ar:read_int()
    local chapter_id = _ar:read_int() 

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    player:get_chapter_award( task_id, chapter_id )
end


function on_carnival_apply_award( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_apply_carnival_award()
    end
end

function on_carnival_apply_box( _ar, _dpid, _size )
    local index = _ar:read_byte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_player_apply_carnival_box( index )
    end
end

function on_open_assist_box( _ar, _dpid, _size )
    local box_type = _ar:read_byte()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:open_assist_box( box_type )
    end
end

function on_get_festival_award( _ar, _dpid, _size )
    local award_type = _ar:read_int()
    local index = _ar:read_int()

    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player:on_get_festival_award( award_type, index )
    end
end

function on_request_stage_star_reward( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

	local page_number = _ar:read_ubyte()
	local stars = _ar:read_ubyte()
	if ( page_number > 0 ) and ( page_number <= quest_worldmap_t.WORLD_MAP_STAGE_PAGE_COUNT ) then
		local ret = player:give_star_count_reward( page_number, stars )	
		player:add_ack_stage_star_reward( ret, page_number )	
	end
end

function on_request_stage_info( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

	local position = _ar:read_int()
	if (position >= quest_worldmap_t.WORLD_MAP_BEGIN_POSITION) and (position <= quest_worldmap_t.WORLD_MAP_END_POSITION) then
		player:add_ack_stage_info( position )	
	end	
end

function on_client_ss_conditiaon_time_out( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
	player:client_condition_timeout()
end

function on_req_palace_use_model( _ar, _dpid, _size )
    local model_index = _ar:read_int()
 
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    player:palace_on_player_select_model( model_index )
end 

-- 排位竞技场
function on_arena_request_info( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local arena_player, opponent_rank = player:arena_request_info()
    if not arena_player or not opponent_rank then
        return
    end
    arena_request_info_resp(_dpid, arena_player, opponent_rank)
	
end

function on_arena_set_battle_team( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local avatar_team = {}

    local avatar_size = _ar:read_ubyte()
    for i = 1, avatar_size do
        local aratar_index = _ar:read_ubyte()
        avatar_team[i] = aratar_index
    end

	local arena_player, error_code = player:arena_set_battle_team(avatar_team)
    arena_set_battle_team_resp(_dpid, avatar_team, arena_player, error_code)

end

function on_arena_start_battle( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local opponent_id = _ar:read_int()

	local error_code = player:arena_start_battle(opponent_id)
    arena_start_battle_resp(_dpid, opponent_id, error_code)
end

function on_arena_get_battle_report( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

	local battle_report = player:arena_get_battle_report()

    arena_get_battle_report_resp(_dpid, battle_report)

end

function on_arena_buy_battle_times( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local buy_times = _ar:read_int()
	local error_code = player:arena_buy_battle_times(buy_times)

    arena_buy_battle_times_resp(_dpid, buy_times, error_code)

end

function on_gm_activity_req_info( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local activity_type = _ar:read_int()
	player:gm_activity_req_info(activity_type)

end

function on_gm_activity_get_reward( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local activity_type = _ar:read_int()
    local id = _ar:read_int()
    local num = _ar:read_int()

	local error_code = player:get_gm_activity_reward(activity_type, id, num)

    player:add_gm_activity_get_reward(activity_type, id, num, error_code)

end

function on_avatar_relic_draw( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )	
    if not player then
        return
    end

    local client_param = _ar:read_int()
    local is_draw_ten = client_param == 1
	local prize_idx_list = player:avatar_relic_draw( is_draw_ten )

    player:add_avatar_relic_draw( prize_idx_list, client_param )	
end

function on_avatar_relic_draw_notify( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local open_or_close = _ar:read_int()
	player:avatar_relic_draw_notify( open_or_close )	
end

function on_avatar_relic_draw_bulletin( _ar, _dpid, _size ) 
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local prize_idx = _ar:read_int()
	player:avatar_relic_draw_bulletin( prize_idx )		
end

function on_pet_draw( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

	local client_param = _ar:read_int()
    local is_draw_ten = client_param == 1
	local prize_idx_list = player:pet_draw( is_draw_ten )

    player:add_pet_draw( prize_idx_list, client_param )	
end

function on_pet_draw_notify( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local open_or_close = _ar:read_int()
	player:pet_draw_notify( open_or_close )	
end

function on_pet_draw_bulletin( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local prize_idx = _ar:read_int()
	player:pet_draw_bulletin( prize_idx )		
end

function on_avatar_chip_draw( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

	local idx = _ar:read_int()
	local job_id = _ar:read_int()

	local item_id, item_num, prize_idx = player:avatar_chip_draw( idx, job_id )

    player:add_avatar_chip_draw( item_id, item_num, prize_idx, job_id )
end

function on_avatar_chip_draw_notify( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

	player:avatar_chip_draw_notify()	
end

function on_get_vip_reward( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local id = _ar:read_int()
    local sub_id = _ar:read_int()
    local index = _ar:read_int()

	local error_code = player:get_vip_reward(id, sub_id, index)

    player:add_get_vip_reward(id, sub_id, index, error_code)

end

function on_get_guild_liveness_gift( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local index = _ar:read_byte()
    player:do_get_guild_liveness_gift(index)
end 

function on_client_sync_tower( _ar, _dpid, _size )
	if not g_mini_server then
		return
	end
	
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
	
	local curr_level = _ar:read_int()
	if not player.tower_ then
		player.tower_ = {}
	end
	player.tower_.current_level_ = curr_level
end

function on_client_sync_palace( _ar, _dpid, _size )
    if not g_mini_server then
        return
    end
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    
    local palace = {}
    palace.open_ = _ar:read_int()
    palace.pass_avatar_index_ = _ar:read_int()
    local used_avatar_indexs = {}
    local len = _ar:read_int()
    for i = 1, len do
        used_avatar_indexs[i] = _ar:read_int()
    end
    palace.used_avatar_indexs_ = used_avatar_indexs
    palace.boss_hp_ = _ar:read_int()
    palace.boss_tige_ = _ar:read_int()
    palace.remain_hp_ = _ar:read_int()
    player.palace_ = palace
end

function on_client_sync_active_avatar_num( _ar, _dpid, _size )
    if not g_mini_server then
        return
    end
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    player.active_avatar_num_ = _ar:read_byte()
end

function on_set_show_vip( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local is_show = _ar:read_int()

    player:set_show_vip(is_show)
    
end

function on_get_bind_account_award( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    player:check_send_bind_account_award()
end

function on_get_this_stage_reward( _ar, _dpid, _size )
	local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
	local stage_no = _ar:read_byte()
	local stars = _ar:read_byte()
    local ret = player:give_stage_reward( stage_no, stars )	
	player:add_get_this_stage_reward( ret )
end


function on_recommend_score( _ar, _dpid, _size )
    local is_agree = _ar:read_byte()

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    if player.recommend_score_index_ == 2 then 
        if is_agree == 0 then 
            player.recommend_score_index_ = 7 
        else 
            local cur_time = os.time()
            cur_time = cur_time + 30 * utils.DAY_SECONDS
            player.recommend_score_index_ =  utils.get_day_index(cur_time)
        end 
    else  
        if is_agree == 0 then 
            local cur_time = os.time()
            cur_time = cur_time + 7 * utils.DAY_SECONDS
            player.recommend_score_index_ =  utils.get_day_index(cur_time)
        else
            local cur_time = os.time()
            cur_time = cur_time + 30 * utils.DAY_SECONDS
            player.recommend_score_index_ =  utils.get_day_index(cur_time)
        end 
    
    end 
end

function on_get_magnate_stage_reward( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local magnate_id = _ar:read_int()
    local index = _ar:read_int()

    local error_code = player:get_magnate_stage_reward(magnate_id, index)

    player:add_get_magnate_stage_reward(magnate_id, index, error_code)

end

function on_get_magnate_rank_reward( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local magnate_id = _ar:read_int()
    local is_today = _ar:read_int()

    local error_code = -100
    if is_today == 1 then
        error_code = player:get_magnate_today_reward(magnate_id)
    else
        error_code = player:get_magnate_total_reward(magnate_id)
    end

    player:add_get_magnate_rank_reward(magnate_id, is_today, error_code)
end

function on_magnate_mall_buy( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local magnate_id = _ar:read_int()
    local index = _ar:read_int()
    local count = _ar:read_int()

    local error_code = player:buy_magnate_goods(magnate_id, index, count)

    player:add_magnate_mall_buy(magnate_id, index, count, error_code)
end

function on_group_buy_request_info( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    player:add_all_group_buy_info()
end

function on_group_buy_buy_good( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local config_id = _ar:read_int()
    local count = _ar:read_int()

    local error_code = player:buy_group_buy_good(config_id, count)

    player:add_buy_group_buy_good(config_id, error_code)
end


function on_resource_fund_claim_reward( _ar, _dpid, _size )

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end

    local day = _ar:read_int()
    local error_code = player:resource_fund_claim_reward(day)
    player:add_resource_fund_claim_reward(error_code)

end

function on_request_enter_exp_circle_instance( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return
    end
    -- 检查系统解锁
    if not system_unlock_mgr.is_unlocked( player, fight_experience_circle.EXP_UNLOCK_SYSTEM_ID ) then
        c_err( "[on_request_enter_exp_circle_instance] UNLOCK FAIL " )
        return
    end 

    fight_experience_circle.on_enter_exp_circle_scene_request( player.player_id_ )
end 



func_table = func_table or {}
func_table[msg.PT_CONNECT] = on_client_connect
func_table[msg.PT_DISCONNECT] = on_client_disconnect
func_table[msg.PT_CERTIFY] = on_client_certify
func_table[msg.PT_GET_RANDOM_NAME] = on_get_random_name
func_table[msg.PT_UNLOCK_RANDOM_NAME] = on_unlock_random_name
func_table[msg.PT_CREATE_PLAYER] = on_create_player
func_table[msg.PT_DELETE_PLAYER] = on_delete_player
func_table[msg.PT_JOIN] = on_join
func_table[msg.PT_RPC] = on_client_remote_call
func_table[msg.PT_SKILL_SLOTS] = on_skill_slots
func_table[msg.PT_SKILL_LEVEL_UP] = on_skill_level_up
func_table[msg.PT_CLIENT_CREATE_PLAYER] = on_client_create_player
func_table[msg.PT_CLIENT_CREATE_MONSTER] = on_client_create_monster
func_table[msg.PT_CLIENT_DELETE_PLAYER] = on_client_delete_player
func_table[msg.PT_CLIENT_REPLACE_DPID] = on_client_replace_dpid
func_table[msg.PT_CLIENT_SYNC_ATTRIBUTE] = on_client_sync_attribute
func_table[msg.PT_CLIENT_SYNC_SKILL] = on_client_sync_skill
func_table[msg.PT_CLIENT_SYNC_ADD_BUFF] = on_client_sync_add_buff
func_table[msg.PT_CLIENT_SYNC_REMOVE_BUFF] = on_client_sync_remove_buff
func_table[msg.PT_CLEAR_ALL_SYNCED_BUFF] = on_clear_all_synced_buff
func_table[msg.PT_CLIENT_SYNC_CITY_POS] = on_client_sync_city_pos
func_table[msg.PT_SELL_ITEM] = on_sell_item
func_table[msg.PT_DO_EQUIP] = on_equip
func_table[msg.PT_USE_ITEM] = on_use_item
func_table[msg.PT_USE_RANDOM_ITEM] = on_use_random_item
func_table[msg.PT_MANUAL_UP_LEVEL] = on_manual_up_level 
func_table[msg.PT_LEARN_TALENT] = on_learn_talent
func_table[msg.PT_JOIN_BATTLEFIELD_MATCH] = on_join_battlefield_match
func_table[msg.PT_QUIT_BATTLEFIELD_MATCH] = on_quit_battlefield_match
func_table[msg.PT_REQ_BATTLEFIELD_STATUS] = on_req_battle_field_status
func_table[msg.PT_REJOIN_BATTLEFIELD] = on_req_rejoin_battle_field
func_table[msg.PT_TAMRIEL_PLAYER_JOIN] = on_tamriel_player_join
func_table[msg.PT_REQ_JOIN_HONOR_HALL_MATCH] = on_req_join_honor_hall_match
func_table[msg.PT_REQ_QUIT_HONOR_HALL_MATCH] = on_req_quit_honor_hall_match
func_table[msg.PT_REQ_HONOR_HALL_RANK] = on_req_honor_hall_rank
func_table[msg.PT_REQ_HONOR_HALL_GET_AWARD] = on_req_honor_hall_get_award
func_table[msg.PT_REQ_HONOR_LADDER_MATCHING_STATE] = on_req_honor_ladder_matching_state
func_table[msg.PT_REQ_HONOR_LADDER_JOIN_MATCH] = on_req_honor_ladder_join_match
func_table[msg.PT_REQ_HONOR_LADDER_QUIT_MATCH] = on_req_honor_ladder_quit_match
func_table[msg.PT_REQ_HONOR_LADDER_CANCEL] = on_req_honor_ladder_cancel
func_table[msg.PT_REQ_HONOR_LADDER_CONFIRM] = on_req_honor_ladder_confirm
func_table[msg.PT_REQ_HONOR_LADDER_MY_RANK] = on_req_honor_ladder_my_rank
func_table[msg.PT_HONOR_TOUR_SIGN_UP] = on_event_honor_tour_sign_up
func_table[msg.PT_HONOR_TOUR_JOIN_BF] = on_event_honor_tour_join_bf
func_table[msg.PT_HONOR_TOUR_QUALIFICATION_PLAYER_LIST] = on_event_honor_tour_qualification_player_list
func_table[msg.PT_JOIN_FIVE_PVP_MATCH] = on_join_five_pvp_match
func_table[msg.PT_QUIT_FIVE_PVP_MATCH] = on_quit_five_pvp_match
func_table[msg.PT_REJOIN_FIVE_PVP] = on_req_rejoin_five_pvp
func_table[msg.PT_ITEM_REBORN] = on_item_reborn
func_table[msg.PT_ITEM_REBORN_OPERATE] = on_item_reborn_operate

-- add by abel
func_table[msg.PT_SOULCARD_REFINE] = on_refine_soulcard
func_table[msg.PT_SOULCARD_DECOMPOSE] = on_decompose_soulcard
func_table[msg.PT_GET_SOULCARD_NEXT_ATTR] = on_get_soulcard_next_attr
func_table[msg.PT_DO_EQUIP_SOULCARD] = on_equip_soulcard

--add by kent
func_table[msg.PT_ITEM_REBORN_NEW] = on_item_reborn_new
func_table[msg.PT_ITEM_REBORN_NEW_LOCK] = on_item_reborn_new_lock
func_table[msg.PT_ITEM_REBORN_NEW_OPERATE] = on_item_reborn_new_operate
--

func_table[msg.PT_ITEM_REBORN_NEW_ATTR_UPGRADE] = on_item_reborn_new_attr_upgrade
func_table[msg.PT_ITEM_REBORN_NEW_ATTR_UPGRADE_OPERATE] = on_item_reborn_new_attr_upgrade_operate

func_table[msg.PT_ITEM_REFINE] = on_item_refine
func_table[msg.PT_ITEM_INHERIT] = on_item_inherit
func_table[msg.PT_ITEM_AUTO_INHERIT] = on_item_auto_inherit
func_table[msg.PT_ITEM_DECOMPOSE] = on_item_decompose
func_table[msg.PT_ITEM_DECOMPOSE_BATCHING] = on_item_decompose_batching
func_table[msg.PT_BOOK_DECOMPOSE] = on_skill_book_decompose
func_table[msg.PT_BOOK_DECOMPOSE_BATCHING] = on_skill_book_decompose_batching
func_table[msg.PT_SKILL_BOOK_COMPOUND] = on_skill_book_compound
func_table[msg.PT_ITEM_ADD_MAGIC] = on_item_add_magic
func_table[msg.PT_ITEM_CONFIRM_MAGIC] = on_item_confirm_magic
func_table[msg.PT_BUY_ITEM_STONE] = on_buy_item_stone
func_table[msg.PT_ORANGE_BUY] = on_buy_orange_fragment
func_table[msg.SM_TYPE_RPC] = on_game_server_remote_call
func_table[msg.SM_CREATE_INSTANCE] = on_mini_server_create_inst
func_table[msg.MS_TYPE_RPC] = on_mini_server_remote_call
func_table[msg.PT_ITEM_RUNE_ABSORB] = on_item_rune_absorb
func_table[msg.PT_PLAYER_RUNE_ACTIVE] = on_player_rune_active
func_table[msg.PT_PLAYER_RUNE_AWAKE] = on_player_rune_awake
func_table[msg.PT_AVATAR_CHANGE_MAIN] = on_player_avatar_change_main
func_table[msg.PT_AVATAR_CHANGE_LIGHT] = on_player_avatar_change_light
func_table[msg.PT_AVATAR_ASCEND_LEVEL] = on_player_avatar_ascend_level
func_table[msg.PT_AVATAR_ASCEND_STAR] = on_player_avatar_ascend_star
func_table[msg.PT_AVATAR_CHANGE_ARTIFACT] = on_player_avatar_change_artifact
func_table[msg.PT_AVATAR_FUSION_ARTIFACT] = on_player_fusion_artifact
func_table[msg.PT_AVATAR_ASCEND_MED_STAR] = on_player_avatar_ascend_med_star
func_table[msg.PT_AVATAR_REBORN_MED_SKILL] = on_player_avatar_reborn_med_skill
func_table[msg.PT_AVATAR_CONFIRM_MED_SKILL] = on_player_avatar_confirm_med_skill
func_table[msg.PT_AVATAR_DESTROY] = on_player_avatar_destroy
func_table[msg.PT_AVATAR_MAT_CONVERSION] = on_player_avatar_material_conversion
--add by kent
func_table[msg.PT_AVATAR_ASCEND_RANK] = on_player_avatar_ascend_rank
--
func_table[msg.PT_SOUL_AWAKE] = on_player_soul_awake
func_table[msg.PT_SOUL_CHANGE_AVATAR] = on_player_soul_change_avatar
func_table[msg.PT_PLAYER_BUY_ENERGY] = on_player_buy_energy
func_table[msg.PT_REQ_QUEST_HEIANZHIMEN_INFO] = on_req_quest_heianzhimen_info
func_table[msg.PT_REQ_QUEST_HEIANZHIMEN_RESET_COUNT] = on_req_quest_heianzhimen_reset_count
func_table[msg.PT_REQ_QUEST_HEIANZHIMEN_START_NEW] = on_req_quest_heianzhimen_start_new
func_table[msg.PT_REQ_QUEST_HEIANZHIMEN_SELECT_DIAMOND_INDEX] = on_req_quest_heianzhimen_select_diamond_index
func_table[msg.PT_REQ_QUEST_HEIANZHIMEN_OPEN_RANDOM_CARD] = on_req_quest_heianzhimen_open_random_card
func_table[msg.PT_REQ_QUEST_HEIANZHIMEN_CLICK_STEP_NEXT] = on_req_quest_heianzhimen_click_step_next
func_table[msg.PT_REQ_QUEST_MAMMONS_TREASURE_RESET] = on_req_quest_mammons_treasure_reset 
func_table[msg.PT_ITEM_STONE_MOUNT] = on_item_stone_mount
func_table[msg.PT_ITEM_STONE_MOUNT_USE_DIAMOND] = on_item_stone_mount_use_diamond 
func_table[msg.PT_ITEM_STONE_UNMOUNT] = on_item_stone_unmount
func_table[msg.PT_SEND_MIGRATE_BEGIN_SUCC] = on_send_migrate_begin_succ
func_table[msg.PT_GOBLIN_MARKET_REDRAW_ALL] = on_goblin_market_redraw_all
func_table[msg.PT_GOBLIN_MARKET_REQUEST_UPDATE] =  on_goblin_market_request_update
func_table[msg.PT_REJOIN] = on_rejoin
func_table[msg.PT_QUIT_LOGIN_QUEUE] = on_quit_login_queue
func_table[msg.PT_REQUEST_SESSION_ALIVE] = on_request_session_alive
func_table[msg.PT_RESELECT_PLAYER] = on_reselect_player
func_table[msg.PT_READ_MAIL] = on_read_mail
func_table[msg.PT_PICK_MAIL] = on_pick_mail
func_table[msg.PT_PRAY_BY_ITEM] = on_pray_by_item
func_table[msg.PT_PRAY_BY_ONE_DIAMOND] = on_pray_one_by_diamond
func_table[msg.PT_PRAY_BY_TEN_DIAMOND] = on_pray_ten_by_diamond
func_table[msg.PT_PRAY_CHECK_EVENT_UPDATE] = on_pray_check_need_update
func_table[msg.PT_RECHARGE_NEW_REQUEST] = on_new_recharge_request
func_table[msg.PT_RECHARGE_CANCEL_REQUEST] = on_cancel_recharge_request
func_table[msg.PT_RECHARGE_COMPLETE_REQUEST] = on_complete_recharge_request
func_table[msg.PT_RECHARGE_CONTINUE_REQUEST] = on_continue_recharge_request
func_table[msg.PT_BUY_PRIVILEGE_BOX] = on_buy_privilege_box
func_table[msg.PT_REQ_INSTANCE_SWEEP] = on_req_instance_sweep
func_table[msg.PT_REQ_PLANE_DROP_AGAIN] = on_req_plane_drop_again
func_table[msg.PT_CLIENT_PREPARE_SPAWN_DATA_ID] = on_client_prepare_spawn_data_id
func_table[msg.PT_REQ_JOIN_ACTIVITY_INSTANCE] = on_req_gameserver_join_activity_instance 
func_table[msg.PT_INSTANCE_PRELOAD_FINISH] = on_instance_preload_finish
func_table[msg.PT_REREQ_JOIN_ACTIVITY_INSTANCE] = on_request_rejoin_activity_instance
func_table[msg.PT_GET_SECRET_AND_TIME] = on_get_secret_and_time
func_table[msg.PT_CONSUME_INSTANCE_COST] = on_consume_instance_cost
func_table[msg.PT_RECONFIRM_QUIT_INSTANCE] = on_reconfirm_quit_instance
func_table[msg.PT_CONFIRM_QUIT_INSTANCE] = on_confirm_quit_instance
func_table[msg.PT_FINISH_MAIN_QUEST] = on_finish_main_quest
func_table[msg.PT_EXCHANGE_PURPLE_SOUL] = on_exchange_purple_soul
func_table[msg.PT_GET_COUPON] = on_get_coupon
func_table[msg.PT_NEWBIE_GUIDE_STATE_FINISH] = on_change_newbie_guide_state_to_finish
func_table[msg.PT_NEWBIE_GUIDE_CURRENT_PROGRESS] = on_notify_newbie_guide_current_progress
func_table[msg.PT_NEWBIE_GUIDE_HISTORY_COND_REC] = on_referesh_newbie_guide_history_cond_rec
func_table[msg.PT_SUSPEND_MST_AI_IN_INSTANCE] = on_suspend_mst_ai_in_instance
func_table[msg.PT_BUY_MYSTIC_GOODS] = on_player_buy_mystic_goods
func_table[msg.PT_REFRESH_MYSTIC_WARE] = on_player_refresh_mystic
func_table[msg.PT_CHECK_UPDATE_MYSTIC] = on_check_need_update_mystic
func_table[msg.PT_ARES_SIGN_UP] = on_event_ares_sign_up
func_table[msg.PT_ARES_JOIN_BF] = on_event_ares_join_bf
func_table[msg.PT_ARES_WITNESS_REQUEST_MATCHES] = on_ares_witness_request_matches
func_table[msg.PT_ARES_WITNESS_ADD_OBSERVE] = on_ares_witness_add_observe
func_table[msg.PT_ARES_WITNESS_REQUEST_WITNESS] = on_ares_witness_request_witness
func_table[msg.PT_HONOR_WITNESS_REQUEST_MATCHES] = on_honor_witness_request_matches
func_table[msg.PT_HONOR_WITNESS_ADD_OBSERVE] = on_honor_witness_add_observe
func_table[msg.PT_HONOR_WITNESS_REQUEST_WITNESS] = on_honor_witness_request_witness
func_table[msg.PT_CREATE_SHOP] = on_create_shop
func_table[msg.PT_PUT_ITEMS_INTO_SHOP] = on_put_items_into_shop
func_table[msg.PT_GET_SHOP_LIST] = on_get_shop_list
func_table[msg.PT_GET_FAVORITE_SHOPS] = on_get_favorite_shops
func_table[msg.PT_GET_SHOP] = on_get_shop
func_table[msg.PT_BUY_ITEM_IN_SHOP] = on_buy_item_in_shop
func_table[msg.PT_WITHDRAW_SHOP_GOLD] = on_withdraw_shop_gold
func_table[msg.PT_PUT_GOLD_INTO_SHOP] = on_put_gold_into_shop
func_table[msg.PT_WITHDRAW_SHOP_ITEM] = on_withdraw_shop_item
func_table[msg.PT_ADD_SHOP_BID_GOLD] = on_add_shop_bid_gold
func_table[msg.PT_GET_SHOP_BID_GOLD_INFO] = on_get_shop_bid_gold_info
func_table[msg.PT_ADD_FAVORITE_SHOP] = on_add_favorite_shop
func_table[msg.PT_RM_FAVORITE_SHOP] = on_rm_favorite_shop
func_table[msg.PT_MODIFY_SHOP_NAME_DESC] = on_modify_shop_name_desc
func_table[msg.PT_MODIFY_SHOP_ITEM_PRICE] = on_modify_shop_item_price
func_table[msg.PT_CLOSE_SHOP] = on_close_shop
func_table[msg.PT_PUT_ITEMS_INTO_WAREHOUSE] = on_put_items_into_warehouse
func_table[msg.PT_WITHDRAW_WAREHOUSE_ITEM] = on_withdraw_warehouse_item
func_table[msg.PT_REQUEST_ACTIVE_TRIGGER] = on_request_active_trigger
func_table[msg.PT_RESET_TOWER] = on_reset_tower
func_table[msg.PT_GET_FIRST_PRIZE] = on_get_tower_first_prize
func_table[msg.PT_EXIT_TO_FINISH] = on_exit_to_finish
func_table[msg.PT_P2P_TRADE_REQUEST] = on_p2p_trade_request
func_table[msg.PT_P2P_TRADE_CANCEL_REQUEST] = on_p2p_trade_cancel_request
func_table[msg.PT_P2P_TRADE_RESPOND] = on_p2p_trade_respond
func_table[msg.PT_P2P_TRADE_CANCEL_TRADE] = on_p2p_trade_cancel_trade
func_table[msg.PT_P2P_TRADE_ADD_ITEM] = on_p2p_trade_add_item
func_table[msg.PT_P2P_TRADE_REMOVE_ITEM] = on_p2p_trade_remove_item
func_table[msg.PT_P2P_TRADE_FIRST_CONFIRM] = on_p2p_trade_first_confirm
func_table[msg.PT_P2P_TRADE_MODIFY_GOLD] = on_p2p_trade_modify_gold
func_table[msg.PT_P2P_TRADE_DEAL_TRADE] = on_p2p_trade_deal
func_table[msg.PT_ATTEND_GET_PRIZE] = on_get_attend_prize
func_table[msg.PT_LEVEL_GET_PRIZE] = on_get_level_prize
func_table[msg.PT_DAILY_GOALS] = on_daily_goals
func_table[msg.PT_PLAYER_RENAME] = on_player_rename
func_table[msg.PT_EXCHANGE_GET_ONSELL_LIST] = on_exchange_get_onsell_list
func_table[msg.PT_EXCHANGE_CREATE] = on_exchange_create
func_table[msg.PT_EXCHANGE_WITHDRAW] = on_exchange_withdraw
func_table[msg.PT_EXCHANGE_RETRIEVE] = on_exchange_retrieve
func_table[msg.PT_EXCHANGE_RECEIVE] = on_exchange_receive
func_table[msg.PT_EXCHANGE_BUY] = on_exchange_buy
func_table[msg.PT_REQ_EVENT_QUESTION_JOIN] = on_req_event_question_join
func_table[msg.PT_REQ_EVENT_QUESTION_QUIT] = on_req_event_question_quit
func_table[msg.PT_REQ_EVENT_QUESTION_SUBMIT] = on_req_event_question_submit
func_table[msg.PT_REQ_EVENT_QUESTION_DOUBLE] = question_on_player_select_double
func_table[msg.PT_REQ_EVENT_PRICE_BOARD_GET_AWARD] = on_req_event_price_board_get_award
func_table[msg.PT_REQ_EVENT_PRICE_BOARD_RESET] = on_req_event_price_board_reset
func_table[msg.PT_REQ_EVENT_PRICE_BOARD_INFO] = on_req_event_price_board_info
func_table[msg.PT_REQ_LUCKY_RATE_DATA] = on_req_player_lucky_rate_data
func_table[msg.PT_ADD_BUFF] = on_add_buff
func_table[msg.PT_REMOVE_BUFF] = on_remove_buff
func_table[msg.PT_GET_LEONA_GIFT] = on_player_get_leona_gift 
func_table[msg.PT_RECHARGE_AWARD_LOTTERY] = on_recharge_award_lottery
func_table[msg.PT_DAILY_ENERGY] = on_apply_daily_energy
func_table[msg.PT_MINISERVER_MONSTER_ONE_DROP] = on_miniserver_get_one_drop
func_table[msg.PT_RUNNER_START] = on_runner_start
func_table[msg.PT_RUNNER_QUIT] = on_runner_quit
func_table[msg.PT_RUNNER_TRADE_INFO] = on_runner_trade_info
func_table[msg.PT_RUNNER_BUY_ITEM] = on_runner_buy_item
func_table[msg.PT_RUNNER_SELL_ITEM] = on_runner_sell_item
func_table[msg.PT_RUNNER_RECEIVE_AWARD] = on_runner_receive_award
func_table[msg.PT_GET_WILD_LIST] = on_get_wild_list
func_table[msg.PT_JOIN_WILD] = on_join_wild
func_table[msg.PT_AUTO_JOIN_WILD] = on_auto_join_wild
func_table[msg.PT_PVE_ELEMENT] = on_pve_element
func_table[msg.PT_PVP_ELEMENT] = on_pvp_element
func_table[msg.PT_ACTIVATE_ELEMENT] = on_activate_element
func_table[msg.PT_DEACTIVATE_ELEMENT] = on_deactivate_element
func_table[msg.PT_UNLOCK_MASTER_SKILL] = on_unlock_master_skill
func_table[msg.PT_UPGRADE_MASTER_SKILL] = on_upgrade_master_skill
func_table[msg.PT_UPGRADE_PRACTICE_SKILL] = on_upgrade_practice_skill
func_table[msg.PT_SYNC_PRACTICE_SKILL] = on_sync_practice_skill
func_table[msg.PT_SYNC_MASTER_SKILL] = on_sync_master_skill
func_table[msg.PT_SYNC_ACTIVE_ELEMENT] = on_sync_active_element
func_table[msg.PT_SYNC_TRANSFORM_MODEL] = on_sync_transform_model
func_table[msg.PT_SYNC_TRANSFORM_SKILL] = on_sync_transform_skill
func_table[msg.PT_SYNC_TRANSFORM_DYN_SKILL] = on_sync_transform_dynamic_skill
func_table[msg.PT_HOLIDAY_OPEN_BOX] = on_holiday_open_box
func_table[msg.PT_GET_RANK_LIST] = on_get_rank_list
func_table[msg.PT_RECHARGE_REWARD_CONSUME] = on_set_recharge_reward_consume_list
func_table[msg.PT_SET_TITLE] = on_set_title
func_table[msg.PT_RECHARGE_REWARD_DIAMOND] = on_get_recharge_reward_diamond
func_table[msg.PT_GET_TRUNCHEON] = on_get_truncheon
func_table[msg.PT_GET_FIRST_RECHARGE_AWARD] = on_get_first_recharge_award
func_table[msg.PT_GUILD_CREATE] = on_guild_create
func_table[msg.PT_GUILD_QUIT] = on_guild_quit
func_table[msg.PT_GUILD_CHANGE_DESC] = on_guild_change_desc
func_table[msg.PT_GUILD_HANDLE_APPLY] = on_guild_handle_apply
func_table[msg.PT_GUILD_SEARCH_BY_NAME] = on_guild_search_by_name
func_table[msg.PT_GUILD_APPLY] = on_guild_apply
func_table[msg.PT_GUILD_HANDLE_INVITE] = on_guild_handle_invite
func_table[msg.PT_GUILD_KICK] = on_guild_kick
func_table[msg.PT_GUILD_SET_ROLE] = on_guild_set_role
func_table[msg.PT_GUILD_CANCEL_APPLY] = on_guild_cancel_apply
func_table[msg.PT_GUILD_GET_LIST] = on_guild_get_list
func_table[msg.PT_GUILD_LEARN_PROFESSION] = on_guild_learn_profession
func_table[msg.PT_GUILD_DROP_PROFESSION] = on_guild_drop_profession
func_table[msg.PT_GUILD_APPLY_PROFESSION] = on_guild_apply_profession
func_table[msg.PT_GUILD_HANDLE_APPLY_PROFESSION] = on_guild_handle_apply_profession
func_table[msg.PT_GUILD_KICK_PROFESSION] = on_guild_kick_profession
func_table[msg.PT_GUILD_PROF_REJECT_GOODS_APPLY] = on_guild_prof_reject_apply
func_table[msg.PT_GUILD_PROF_APPLY_GOODS] = on_guild_prof_apply_goods
func_table[msg.PT_GUILD_PROF_DELIVERY_GOODS] = on_guild_prof_delivery_goods
func_table[msg.PT_GUILD_PROF_COMPOSITE_GOODS] = on_guild_prof_composite_goods
func_table[msg.PT_GUILD_ENTER_GUILD_SCENE] = on_guild_enter_guild_scene
func_table[msg.PT_GUILD_PROF_ACTIVATE_GOODS] = on_guild_prof_activate_goods
func_table[msg.PT_GUILD_PROF_CANCEL_GOODS] = on_guild_prof_cancel_goods_effect
func_table[msg.PT_GUILD_DONATE_MONEY ] = on_guild_donate_money
func_table[msg.PT_GUILD_DONATE_DIAMOND ] = on_guild_donate_diamond
func_table[msg.PT_GUILD_DONATE_SET_ACTIVATE_ID ] = on_donate_set_activate_id
func_table[msg.PT_GUILD_RENAME ] = on_guild_rename
func_table[msg.PT_ENROLL_WEEKEND_PVP] = on_player_enroll_weekend_pvp
func_table[msg.PT_JOIN_WEEKEND_PVP_BATTLE] = on_request_join_weekend_pvp_battle 
func_table[msg.PT_OPEN_WEEKLY_TASK] = on_open_weekly_task
func_table[msg.PT_FINISH_WEEKLY_TASK] = on_finish_weekly_task
func_table[msg.PT_GET_TASK_QUESTION] = on_get_task_question
func_table[msg.PT_ANSWER_TASK_QUESTION] = on_answer_task_question
func_table[ msg.PT_ENTER_TEAM_INST ] = on_enter_team_inst
func_table[msg.PT_TRUNCHEON_REFINE] = on_truncheon_refine
func_table[msg.PT_TRUNCHEON_REBORN] = on_truncheon_reborn
func_table[msg.PT_TRUNCHEON_REBORN_CONFIRM] = on_truncheon_reborn_confirm
func_table[msg.PT_TRUNCHEON_LETTERING] = on_truncheon_lettering
func_table[msg.PT_TRUNCHEON_LETTERING_CONFIRM] = on_truncheon_lettering_confirm
func_table[msg.PT_TRUNCHEON_REBORN_EXCHANGE] = on_truncheon_reborn_exchange
func_table[msg.PT_FIVE_PVP_INVITE_FRIEND] = on_five_pvp_invite_friend
func_table[msg.PT_FIVE_PVP_INVITE_ACCEPT] = on_five_pvp_invite_accept
func_table[msg.PT_FIVE_PVP_INVITE_REFUSE] = on_five_pvp_invite_refuse
func_table[msg.PT_FIVE_PVP_QUIT_TEAM] = on_five_pvp_quit_team
func_table[msg.PT_FIVE_PVP_KICK_TEAMMATE] = on_five_pvp_kick_teammate
func_table[msg.PT_FIVE_PVP_REQ_TEAM_INFO] = on_five_pvp_request_team_info
func_table[msg.PT_FIVE_PVP_CLEAR_REMINDER] = on_five_pvp_clear_reminder
func_table[msg.PT_FIVE_PVP_JOIN_TEAM_MATCH] = on_five_pvp_join_team_match
func_table[msg.PT_LEONA_PRESENT] = on_leona_present
func_table[msg.PT_REQUEST_EXCHANGE_EQUIP] = on_request_exchange_equip
func_table[msg.PT_REQUEST_EQUIP_EXCHANGE_SHOP_ITEMS] = on_request_equip_exchange_shop_items

func_table[msg.PT_WEEKEND_PVP_MEMBER_CONFIRM] = on_member_confirm_weekend_pvp_enroll
func_table[msg.PT_ENTER_MILLIONAIRE_INST] = on_ack_enter_millionaire_inst
func_table[msg.PT_MILLIONAIRE_ACK_STATUS] = on_req_millionaire_status
func_table[msg.PT_MILLIONAIRE_REJOIN] = on_req_rejoin_millionaire 
func_table[msg.PT_MILLIONAIRE_ACK_PLAYER_POS] = on_ack_millionaire_player_pos 

func_table[msg.PT_ORANGE_EQUIP_PREVIEW] = on_orange_equip_preview
func_table[msg.PT_ORANGE_EQUIP_UPGRADE] = on_orange_equip_upgrade
func_table[msg.PT_ORANGE_EQUIP_EXCHANGE] = on_orange_equip_exchange
func_table[msg.PT_ONE_KEY_EQUIP] = on_one_key_equip

func_table[msg.PT_ORANGE_EQUIP_ASCEND_PREVIEW] = on_orange_equip_ascend_preview
func_table[msg.PT_ORANGE_EQUIP_ASCEND] = on_orange_equip_ascend

func_table[msg.PT_ORANGE_EQUIP_AWAKE] = on_orange_equip_awake
func_table[msg.PT_ORANGE_EQUIP_AWAKE_PREVIEW] = on_orange_equip_awake_preview
func_table[msg.PT_ORANGE_MATERIAL_MELT] = on_orange_material_melt

func_table[msg.PT_HONOR_HALL_INVITE_PK] = on_honor_hall_invite_pk
func_table[msg.PT_HONOR_HALL_ACK_PK] = on_honor_hall_ack_pk

func_table[ msg.PT_REJOIN_TEAM_INST ] = on_team_inst_rejoin
func_table[ msg.PT_STRONGER_GS_GRADE ] = on_stronger_gs_grade
func_table[ msg.PT_WELFARE_BUY ] = on_welfare_buy

func_table[ msg.PT_RECHARGE_REMIND_OPEN] = on_recharge_remind_open
func_table[ msg.PT_FETCH_PLAYER_ATTR] = on_fetch_player_attr
func_table[ msg.PT_PUTAWAY_ITEM ]  = on_putaway_item
func_table[ msg.PT_BUY_TRADE_ITEM ]  = on_buy_trade_item
func_table[ msg.PT_SOLD_OUT_ITEM ]  = on_sold_out_item
func_table[ msg.PT_QUERY_TRADE_ITEM_LIST ]  = on_query_trade_item_list
func_table[ msg.PT_MODIFY_TRADE_PRICE ]  = on_modify_trade_price
func_table[ msg.PT_ADD_FAVORITE_ITEM ]  = on_add_favorite_item
func_table[ msg.PT_RM_FAVORITE_ITEM ]  = on_remove_favorite_item
func_table[ msg.PT_GET_FAVORITE_ITEM_LIST ]  = on_get_favorite_item_list
func_table[ msg.PT_APPOINT_WITTNIGHT ] = on_appoint_wittnight

func_table[ msg.PT_BUY_GOLD ] = on_buy_gold
func_table[ msg.PT_BUY_GOLD_UI_OPENED ] = on_buy_gold_ui_opened

func_table[ msg.PT_ELAM_MILITARY_SUP_FREE ] = on_elam_military_supplies_draw_free
func_table[ msg.PT_ELAM_MILITARY_SUP_ONCE ] = on_elam_military_supplies_draw_once
func_table[ msg.PT_ELAM_MILITARY_SUP_TEN ] = on_elam_military_supplies_draw_ten

func_table[ msg.PT_ONE_GOLD_LUCKY_DRAW_I_WANT_IN ] = on_join_one_gold_lucky_draw
func_table[ msg.PT_OPEN_ONE_GOLD_LUCKY_DRAW_UI ] = on_open_one_gold_lucky_draw_ui
func_table[ msg.PT_ONE_GOLD_LUCKY_DRAW_EXCHANGE_BONUS ] = on_one_gold_lucky_draw_exchange_bonus

func_table[ msg.PT_COMPETITIVE_HALO ] = on_add_competitive_halo
func_table[ msg.PT_ROBOT_ADD_ITEM ] = on_robot_add_item

func_table[ msg.PT_GUILD_DAILY_PVP_ENROLL ] = on_guild_daily_pvp_enroll
func_table[ msg.PT_GUILD_DAILY_PVP_JOIN_BATTLE ] = on_guild_daily_pvp_join_battle
func_table[ msg.PT_GUILD_DAILY_PVP_CREATE_TEAM] = on_guild_daily_pvp_create_team
func_table[ msg.PT_GUILD_DAILY_PVP_DELETE_TEAM] = on_guild_daily_pvp_delete_team
func_table[ msg.PT_GUILD_DAILY_PVP_JOIN_TEAM] = on_guild_daily_pvp_join_team
func_table[ msg.PT_GUILD_DAILY_PVP_QUIT_TEAM] = on_guild_daily_pvp_quit_team
func_table[ msg.PT_GUILD_DAILY_PVP_KICK_TEAM] = on_guild_daily_pvp_kick_team

func_table[ msg.PT_AUCTION_ITEM ] = on_auction_item
func_table[ msg.PT_CANCEL_AUCTION_ITEM ] = on_cancel_auction_item
func_table[ msg.PT_QUERY_AUCTION_ITEM ] = on_query_auction_item
func_table[ msg.PT_BID_AUCTION_ITEM ] = on_bid_auction_item   
func_table[ msg.PT_GET_BID_ITEM ] =  on_get_bid_item       
func_table[ msg.PT_GET_AUCTION_STATISTIC ] = on_get_item_auction_statistic

func_table[ msg.PT_GUILD_PVE_START] = on_guild_pve_start
func_table[ msg.PT_GUILD_PVE_NOTIFY] = on_guild_pve_notify

func_table[msg.PT_GW_ENROLL_BATTLE] = on_guild_weekend_enroll
func_table[msg.PT_GW_ENTER_BATTLE] = on_guild_weekend_enter

func_table[msg.PT_OPEN_BOUTIQUE] = on_open_guild_boutique
func_table[msg.PT_BUY_BOUTIQUE_ITEM] = on_buy_boutique_item
func_table[msg.PT_ASK_PAY_BOUTIQUE_ITEM] = on_ask_for_pay_boutique_item
func_table[msg.PT_PAY_BOUTIQUE_ITEM] =  on_pay_boutique_item_for_others

func_table[msg.PT_GOD_DIVINATION_DRAW_RUNES] =  on_god_divination_draw_runes
func_table[msg.PT_GOD_DIVINATION_BUY_ENERGY] =  on_god_divination_buy_energy

func_table[msg.PT_GET_LEVEL_REWARD ] =  on_get_level_reward

func_table[msg.PT_PET_UNLOCK] =  on_pet_unlock
func_table[msg.PT_PET_ACTIVE] =  on_pet_active
func_table[msg.PT_PET_RENAME] =  on_pet_rename
func_table[msg.PT_PET_UPGRADE] = on_pet_upgrade

func_table[msg.PT_HOLIDAY_SIGNIN_AWARD_GET] = on_get_holiday_signin_award 
func_table[msg.PT_GET_MONTHLY_SIGNIN_AWARD] = on_get_monthly_signin_award 
func_table[msg.PT_GET_DAILY_ONLINE_AWARD] = on_get_daily_online_award 
func_table[msg.PT_GET_CYCLED_RECHARGE_AWARD] = on_get_cycled_recharge_award 
func_table[msg.PT_BUY_GROWTH_FUND] = on_buy_growth_fund 
func_table[msg.PT_GET_GROWTH_FUND] = on_get_growth_fund 
func_table[msg.PT_MALL_BUY_PRODUCT] = on_buy_mall_product
func_table[msg.PT_TEAM_LADDER_CREATE_TEAM] = on_req_create_ladder_team
func_table[msg.PT_TEAM_LADDER_APPLY_JOIN] = on_apply_join_ladder_team
func_table[msg.PT_TEAM_LADDER_REQ_TEAM_LIST] = on_req_ladder_team_list
func_table[msg.PT_TEAM_LADDER_REQ_TEAM_INFO] = on_req_ladder_team_info
func_table[msg.PT_TEAM_LADDER_KICK_MEMBER] = on_kick_ladder_team_member 
func_table[msg.PT_TEAM_LADDER_REQ_TEAM_APPLY_LIST] = on_req_ladder_team_apply_list 
func_table[msg.PT_TEAM_LADDER_HANDLE_JOIN_APPLY] = on_handle_ladder_team_join_apply 
func_table[msg.PT_TEAM_LADDER_REQ_QUIT_TEAM] = on_req_quit_ladder_team 
func_table[msg.PT_TEAM_LADDER_DEL_NOT_FULL_TEAM_NOTIFY] = on_del_not_full_ladder_team_notify
func_table[msg.PT_TEAM_LADDER_CHANGE_TEAM_TEXT] = on_change_ladder_team_text
func_table[msg.PT_GET_LOGIN_AWARD] = on_get_login_award
func_table[msg.PT_GET_TASK_AWARD] = on_get_login_task_award
func_table[msg.PT_GET_RECHARGE_AWARD] = on_get_recharge_award
func_table[msg.PT_BUY_HALF_PRICE_ITEM] = on_buy_half_price_item
func_table[msg.PT_TEAM_LADDER_REQ_JOIN_MATCH] = on_req_join_event_team_ladder_match
func_table[msg.PT_TEAM_LADDER_CONFIRM_MATCH] = on_confirm_event_team_ladder_match
func_table[msg.PT_TEAM_LADDER_REQ_JOIN_BATTLE] = on_req_join_event_team_ladder_battle
func_table[msg.PT_TEAM_LADDER_INVITE_FRIEND] = on_invite_friend_join_ladder_team
func_table[msg.PT_TEAM_LADDER_HANDLE_INVITE] = on_handle_ladder_team_invite
func_table[msg.PT_TEAM_LADDER_SEARCH_TEAM] = on_search_not_full_team
func_table[msg.PT_TEAM_LADDER_REQUEST_RECRUIT] = on_req_ladder_team_recruit 
func_table[msg.PT_REQ_TEAM_LADDER_RANK] = on_req_team_ladder_rank
func_table[msg.PT_GET_CELEBRATION_AWARD] = on_get_celebration_award
func_table[msg.PT_CELE_LOTTERY_ONE] = on_cele_lottery_one
func_table[msg.PT_CELE_LOTTERY_TEN] = on_cele_lottery_ten
func_table[msg.PT_CELE_INVEST] = on_cele_invest
func_table[msg.PT_CHECK_LOTTERY_UPDATE] = on_check_lottery_update
func_table[msg.PT_LUCKY_RANK] = on_buy_lottery_mall_product
func_table[msg.PT_TRANSFER_ELAM_MONEY_2_VOUCHER] = on_transfer_elam_money_2_voucher
func_table[msg.PT_TRANSFER_VOUCHER_2_DIAMOND] = on_transfer_voucher_2_diamond

func_table[msg.PT_SANDING] =  on_sanding
func_table[msg.PT_OPEN_OFFICER_BOX] =  on_open_officer_box

func_table[msg.PT_SYN_MALL_PRODUCT] =  on_ask_syn_mall_product

func_table[msg.PT_QUICK_MATCH_INST] =  on_quick_match_inst
func_table[msg.PT_CONFIRM_QUICK_MATCH] =  on_confirm_quick_match
func_table[msg.PT_CANCEL_QUICK_MATCH] =  on_cancel_quick_match
func_table[msg.PT_COMMIT_MIPUSH_REGID] = on_commit_mipush_regid
func_table[msg.PT_GET_TOWER_RANK_LIST] = on_get_tower_rank_list
func_table[msg.PT_GET_PALACE_RANK_LIST] = on_get_palace_rank_list

func_table[msg.PT_HALL_TRY_REFRESH] = on_hall_try_refresh
func_table[msg.PT_HALL_UPVOTE] = on_hall_upvote
func_table[msg.PT_HALL_UPVOTE_DATA] = on_hall_upvote_data
func_table[msg.PT_HALL_SET_GREETING_AND_DIALOGUE] = on_hall_set_greeting_and_dialogue
func_table[msg.PT_HALL_UPVOTE_RANK_DATA] = on_hall_upvote_rank_data

func_table[msg.PT_TEAM_CHAMPION_REQUEST_ENROLL] = on_request_team_champion_enroll
func_table[msg.PT_TEAM_CHAMPION_REQUEST_QUALIFICATION_LIST] = on_request_team_champion_qualification_list
func_table[msg.PT_TEAM_CHAMPION_REQUEST_MATCH_RECORD] = on_request_team_champion_match_record
func_table[msg.PT_TEAM_CHAMPION_DELETE_FOLLOW_MATCH_PLAYER] = on_team_champion_delete_follow_match_player
func_table[msg.PT_TEAM_CHAMPION_REQUEST_JOIN_BATTLE] = on_team_champion_request_join_battle
func_table[msg.PT_TEAM_CHAMPION_REQUEST_WATCH_MATCH] = on_team_champion_request_watch_match
func_table[msg.PT_TEAM_CHAMPION_REQUEST_MATCH_RANK] = on_team_champion_request_match_rank

func_table[msg.PT_QUERY_REBATE_INFO] = on_query_rebate_info
func_table[msg.PT_CONFIRM_REBATE] = on_confirm_rebate

func_table[msg.PT_CHAPTER_TASK_AWARD] = on_get_chapter_award 

func_table[msg.PT_CARNIVAL_APPLY_AWARD] = on_carnival_apply_award
func_table[msg.PT_CARNIVAL_APPLY_BOX] = on_carnival_apply_box

func_table[msg.PT_OPEN_ASSIST_BOX] = on_open_assist_box
func_table[msg.PT_GET_FESTIVAL_AWARD] = on_get_festival_award

func_table[msg.PT_OPEN_CHAT_GIFT_BOX] = on_open_chat_gift_box
func_table[msg.PT_GET_CHRISTMAS_SELL_AWARD] = on_get_christmas_sell_award
func_table[msg.PT_OPEN_CHRISTMAS_RECHARGE_LUCKY_DRAW_BOX] = on_open_christmas_recharge_lucky_draw_box
func_table[msg.PT_UPGRADE_WING] = on_upgrade_wing

func_table[msg.PT_WING_CORE_UPGRADE] = on_wing_core_upgrade
func_table[msg.PT_WING_CORE_EQUIP] = on_wing_core_equip
func_table[msg.PT_WING_SKILL_UNLOCK] = on_wing_skill_unlock
func_table[msg.PT_WING_SKILL_UPGRADE] = on_wing_skill_upgrade

func_table[msg.PT_REQUEST_STAGE_STAR_REWARD] = on_request_stage_star_reward
func_table[msg.PT_STAGE_INFO] = on_request_stage_info
func_table[msg.PT_SS_CONDITION_TIME_OUT] = on_client_ss_conditiaon_time_out

func_table[msg.PT_REQ_PALACE_USE_MODEL] = on_req_palace_use_model

func_table[msg.PT_ARENA_REQUEST_INFO] = on_arena_request_info
func_table[msg.PT_ARENA_SET_BATTLE_TEAM] = on_arena_set_battle_team
func_table[msg.PT_ARENA_START_BATTLE] = on_arena_start_battle
func_table[msg.PT_ARENA_GET_BATTLE_REPORT] = on_arena_get_battle_report
func_table[msg.PT_ARENA_BUY_BATTLE_TIMES] = on_arena_buy_battle_times

-- func_table[msg.PT_GM_ACTIVITY_REQ_INFO] = on_gm_activity_req_info
func_table[msg.PT_GM_ACTIVITY_GET_REWARD] = on_gm_activity_get_reward
func_table[msg.PT_PLAYER_ASTROLABE_ASCEND_LEVEL] = on_player_astrolabe_ascend_level
func_table[msg.PT_PLAYER_ASTROLABE_OPER_AVATAR] = on_player_astrolabe_oper_avatar

func_table[msg.PT_GET_VIP_REWARD] = on_get_vip_reward
func_table[msg.PT_GET_GUILD_LIVENESS_GIFT] = on_get_guild_liveness_gift

func_table[msg.PT_GET_GUILD_LIVENESS_GIFT] = on_get_guild_liveness_gift


--func_table[msg.PT_QUERY_REFINE_ATTR] = on_query_refine_attr

func_table[msg.PT_AVATAR_RELIC_DRAW] = on_avatar_relic_draw
func_table[msg.PT_AVATAR_RELIC_DRAW_NOTIFY] = on_avatar_relic_draw_notify
func_table[msg.PT_AVATAR_RELIC_DRAW_BULLETIN] = on_avatar_relic_draw_bulletin
func_table[msg.PT_PET_DRAW] = on_pet_draw
func_table[msg.PT_PET_DRAW_NOTIFY] = on_pet_draw_notify
func_table[msg.PT_PET_DRAW_BULLETIN] = on_pet_draw_bulletin

func_table[msg.PT_AVATAR_CHIP_DRAW] = on_avatar_chip_draw
func_table[msg.PT_AVATAR_CHIP_DRAW_NOTIFY] = on_avatar_chip_draw_notify

func_table[msg.PT_SET_SHOW_VIP] = on_set_show_vip
func_table[msg.PT_CLIENT_SYNC_TOWER] = on_client_sync_tower
func_table[msg.PT_GET_BIND_ACCOUNT_AWARD] = on_get_bind_account_award
func_table[msg.PT_CLIENT_SYNC_PALACE] = on_client_sync_palace
func_table[msg.PT_CLIENT_SYNC_ACTIVE_AVATAR_NUM] = on_client_sync_active_avatar_num

func_table[msg.PT_GET_THIS_STAGE_REWARD] = on_get_this_stage_reward

func_table[msg.PT_RECOMMEND_SCORE] = on_recommend_score

func_table[msg.PT_GET_MAGNATE_STAGE_REWARD] = on_get_magnate_stage_reward
func_table[msg.PT_GET_MAGNATE_RANK_REWARD] = on_get_magnate_rank_reward
func_table[msg.PT_MAGNATE_MALL_BUY] = on_magnate_mall_buy

func_table[msg.PT_GROUP_BUY_REQUEST_INFO] = on_group_buy_request_info
func_table[msg.PT_GROUP_BUY_BUY_GOOD] = on_group_buy_buy_good

-- 资源基金
func_table[msg.PT_RESOURCE_FUND_CLAIM_REWARD] = on_resource_fund_claim_reward

func_table[msg.PT_REQUEST_ENTER_EXP_CIRCLE_INSTANCE] = on_request_enter_exp_circle_instance

function serialize_server_info(_ar)
    _ar:flush_before_send( msg.ST_SERVER_INFO ) 
    _ar:write_boolean( g_debug )
    _ar:write_boolean( player_t.g_open_newbie_guide )
    _ar:write_string( g_gamesvr:c_get_game_area() )
    _ar:write_boolean( chat_mng.ENABLE_VOICE )

    -- 是否是国服
    _ar:write_boolean(g_gamesvr:c_is_china())
end

function send_already_login_to_client( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.ST_ALREADY_LOGIN_OTHER )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_certify_code_to_client( _dpid, _retcode, _certify_key, _ext_param )
    local ar = g_ar         
    ar:flush_before_send( msg.ST_CERTIFY_RESULT )
    ar:write_ushort( _retcode )
    ar:write_int( _certify_key )
    ar:write_int( _ext_param or 0 )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_login_queue_is_full( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOGIN_QUEUE_FULL ) 
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_login_queue_enqueue( _dpid, _rank )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOGIN_QUEUE_ENQUEUE )
    ar:write_ushort( _rank )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_login_queue_rank( _dpid, _rank )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOGIN_QUEUE_RANK )
    ar:write_ushort( _rank )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_login_queue_quit_ret( _dpid, _ret )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOGIN_QUEUE_QUIT_RET ) 
    ar:write_ubyte( _ret and 1 or 0 )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_session_alive_ret( _dpid, _alive )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOGIN_SESSION_ALIVE_RET )
    ar:write_ubyte( _alive and 1 or 0 )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_close_session_ret( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOGIN_CLOSE_SESSION_RET )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_login_error( _dpid, _error_code )
    local ar = g_ar
    ar:flush_before_send( msg.ST_LOIGN_ERR )
    ar:write_ubyte( _error_code )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_reconnect_from_bf( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.ST_RECONNECT_FROM_BF )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_do_join_bf_state_err( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.ST_DOJOIN_BF_STATE_ERR )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_player_list_to_client( _dpid, _account_info )
    local ar = g_ar         
    ar:flush_before_send( msg.ST_PLAYERLIST_RESULT )
    local player_cnt = _account_info.player_cnt_
    ar:write_ubyte( player_cnt )
    local player_list = _account_info.player_list_
    for i = 1, player_cnt, 1 do
        local pinfo = player_list[i]
        ar:write_ulong( pinfo.player_id_ )     
        ar:write_ulong( pinfo.model_id_ )  
        ar:write_byte( pinfo.use_model_ )
        ar:write_byte( pinfo.model_level_ )
        ar:write_ulong( pinfo.job_id_ )
        ar:write_string( pinfo.player_name_ )   
        ar:write_ulong( pinfo.level_ )
        ar:write_ulong( pinfo.slot_ )
        ar:write_ulong( pinfo.total_gs_ )
        ar:write_ulong( pinfo.wing_id_ )
        ar:write_string( pinfo.active_weapon_sfxs_ )
        ar:write_byte( pinfo.need_download )
        --c_err("[send_player_list_to_client] active_weapon_sfxs_: %s", pinfo.active_weapon_sfxs_)
    end
    ar:write_ulong( _account_info.rand_key_ )
    ar:write_ulong( _account_info.last_join_player_id_ )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_random_name_to_client( _dpid, _player_name )
    local ar = g_ar         
    ar:flush_before_send( msg.ST_RANDOM_NAME )
    ar:write_string( _player_name )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_create_player_result_to_client( _dpid, _retcode, _player_id, _player_name )
    local ar = g_ar         
    ar:flush_before_send( msg.ST_CREATE_PLAYER_RESULT )
    ar:write_int( _retcode )
    ar:write_int( _player_id )
    ar:write_string( _player_name )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_delete_player_result_to_client( _dpid, _retcode )
    local ar = g_ar         
    ar:flush_before_send( msg.ST_DELETE_PLAYER_RESULT )
    ar:write_ubyte( _retcode )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_player_join_result_to_client( _dpid, _retcode )
    local ar = g_ar         
    ar:flush_before_send( msg.ST_JOIN_PLAYER_RESULT )
    ar:write_ubyte( _retcode )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_manual_up_level_ack( _dpid , _retcode )
    local ar = g_ar 
    ar:flush_before_send(msg.ST_MANUAL_UP_LEVEL_ACK)
    ar:write_int ( _retcode )
    ar:send_one_ar( g_gamesvr , _dpid )
end

function send_chamber_commerce_info( _dpid, _is_chamber, _is_fetched, _is_buy_success, _diamond, _item_list, _is_send_mail ) 
    local ar = g_ar
    ar:flush_before_send( msg.ST_CHAMBERCOMMERCE_RESET ) 
    ar:write_byte( _is_chamber ) 
    ar:write_byte( _is_fetched ) 
    ar:write_byte( _is_buy_success or 0 ) 
    ar:write_short( _diamond or 0 ) 
    if _item_list then 
        ar:write_byte( #_item_list ) 
        for _, id in pairs( _item_list ) do 
            ar:write_int( id )
        end
    else
        ar:write_byte( 0 )
    end
    ar:write_byte( _is_send_mail and 0x01 or 0x00 )
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_earl_pass_days( _dpid, _pass_days, _total_days )
    local ar = g_ar
    ar:flush_before_send( msg.ST_EARL_PASS_DAYS) 
    ar:write_byte( _pass_days ) 
    ar:write_byte( _total_days ) 
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_wild_list_to_client( _dpid, _wild_type, _page_num, _page_index, _max_player_num, _result_list )
    local ar = g_ar
    ar:flush_before_send( msg.ST_WILD_LIST ) 
    ar:write_int( _wild_type )
    ar:write_int( _page_num )
    ar:write_int( _page_index )
    ar:write_int( _max_player_num )
    ar:write_int( #_result_list )
    for _, wild in ipairs( _result_list ) do
        ar:write_int( wild[1] ) -- wild_id
        ar:write_int( wild[2] ) -- player_num
		ar:write_int( wild[3] ) -- has_boss
    end
    ar:send_one_ar( g_gamesvr, _dpid )
end

function send_clear_enter_inst_record( _dpid ) 
    local ar = g_ar 
    ar:flush_before_send( msg.ST_SYN_ENTER_INST_RECORD ) 
    ar:send_one_ar( g_gamesvr, _dpid ) 
end

function send_enter_inst_confirm( _dpid ) 
    local ar = g_ar 
    ar:flush_before_send( msg.ST_SYN_ENTER_INST_CONFIRM ) 
    ar:send_one_ar( g_gamesvr, _dpid ) 
end

function send_server_info( _dpid )
    local ar = g_ar 
    serialize_server_info(ar)
    ar:send_one_ar( g_gamesvr, _dpid ) 
end

function arena_request_info_resp(_dpid, _arena_player, _opponent_rank)
    local ar = g_ar 
    ar:flush_before_send(msg.ST_ARENA_REQUEST_INFO)

    ar:write_int(_arena_player.rank_)
    ar:write_int(_arena_player.buy_times_ + ARENA_SETTING.ArenaInitPoint[1] - _arena_player.battled_times_)
    ar:write_int(_arena_player.buy_times_)

    local opponent_size = utils.table.size(_opponent_rank)
    ar:write_int(opponent_size)
    for k, v in ipairs(_opponent_rank) do
        ar:write_int(v.player_id_)
        ar:write_int(v.rank_)
        ar:write_int(v.head_icon_id_)
        ar:write_int(v.level_)
        ar:write_string(v.player_name_)
        ar:write_int(v.total_gs_)
    end

    ar:write_int(#_arena_player.team_)
    for _,v in ipairs(_arena_player.team_) do
        ar:write_ubyte(v)
    end

    ar:send_one_ar(g_gamesvr, _dpid)
end


function arena_set_battle_team_resp(_dpid, _avatar_team, _arena_player, _error_code)
    local ar = g_ar 
    ar:flush_before_send(msg.ST_ARENA_SET_BATTLE_TEAM)

    ar:write_int(_error_code)

    ar:write_int(#_avatar_team)
    for i= 1, #_avatar_team do
        ar:write_ubyte(_avatar_team[i])
    end

    ar:send_one_ar(g_gamesvr, _dpid)
end

function arena_start_battle_resp(_dpid, _opponent_id, _error_code)
    local ar = g_ar 
    ar:flush_before_send(msg.ST_ARENA_START_BATTLE)

    ar:write_int(_error_code)
    -- ar:write_int(_opponent_id)

    ar:send_one_ar(g_gamesvr, _dpid)
end


function arena_get_battle_report_resp(_dpid, _battle_report)
    
    local ar = g_ar 
    ar:flush_before_send(msg.ST_ARENA_GET_BATTLE_REPORT)

    ar:write_int(#_battle_report)
    for i= 1, #_battle_report do
        local battle_report = _battle_report[i]
        ar:write_int(battle_report.is_win_)
        ar:write_int(battle_report.rank_before_)
        ar:write_int(battle_report.rank_after_)
        ar:write_int(battle_report.opponent_head_icon_id_ or 10009)
        ar:write_string(battle_report.opponent_name_)
        ar:write_int(battle_report.battle_time_)
        ar:write_int(battle_report.opponent_gs_)
    end

    ar:send_one_ar(g_gamesvr, _dpid)

end

function arena_buy_battle_times_resp(_dpid, buy_times, error_code)
    
    local ar = g_ar 
    ar:flush_before_send(msg.ST_ARENA_BUY_BATTLE_TIMES)

    ar:write_int(buy_times)
    ar:write_int(error_code)

    ar:send_one_ar(g_gamesvr, _dpid)

end

