module( "gamesvr", package.seeall )

local sformat = string.format

function do_join( _migrating_player )
    local dpid = _migrating_player.dpid_
    if not dpid then  -- 检查客户端是否已连上bf并通过验证
        return
    end

    local player = _migrating_player.player_
    if not player then  -- 检查bf是否已收到game发送的玩家数据
        return
    end

    -- do join
    local server_id = _migrating_player.server_id_
    local player_id = player.player_id_
    c_log( "[gamesvr](do_join) server_id: %d, player_id: %d, do join", server_id, player_id )
    g_dpid_mng[dpid] = player.dpid_info_
    player.dpid_info_ = nil
    bfsvr.remove_migrating_player( server_id, player_id )
    player_t.unserialize_before_join( player )
    player_mng.add_player( server_id, player, dpid )

    for player_data_op_id, player_data_op in pairs( _migrating_player.player_data_ops_ ) do
        -- 先调用bfclient.remove_player_data_op删除game那边的数据，然后再调用func_for_online_player，
        -- 这样万一func_for_online_player在中途抛出异常，也不至于因为bfclient.remove_player_data_op没执行到，
        -- 而导致在玩家从bf下线后，game那边又将data_op执行了一遍，使得func_for_online_player已经添加了一部分
        -- 的数据又被重复添加一遍。
        bfsvr.remote_call_game( server_id, "bfclient.remove_player_data_op", player_id, player_data_op_id )
        player_data_op.func_for_online_player_( player, player_data_op.params_ )
    end
end

function on_ask_bf_register_dpid( _ar, _dpid, _size )
    local gamesvr_id = _ar:read_int()
    local player_id = _ar:read_ulong()
    local bf_certify_code = _ar:read_int()

    local migrating_player = bfsvr.get_migrating_player( gamesvr_id, player_id )
    if not migrating_player then
        c_err( "[gamesvr](on_ask_bf_register_dpid) dpid: 0x%X, gamesvr_id: %d, player_id: %d, no such migrating player", _dpid, gamesvr_id, player_id )
        g_gamesvr:c_kick_player( _dpid )
        return
    end

    if migrating_player.bf_certify_code_ ~= bf_certify_code then
        c_err( "[gamesvr](on_ask_bf_register_dpid) dpid: 0x%X, gamesvr_id: %d, player_id: %d, bf_certify_code not match, expect: %d, received: %d",
               _dpid, gamesvr_id, player_id, migrating_player.bf_certify_code_, bf_certify_code )
        g_gamesvr:c_kick_player( _dpid )
        return
    end

    c_log( "[gamesvr](on_ask_bf_register_dpid) dpid: 0x%X, gamesvr_id: %d, player_id: %d, player is ready to join", _dpid, gamesvr_id, player_id )
    bfsvr.g_dpid2migrating_players[_dpid] = migrating_player
    migrating_player.dpid_ = _dpid
    do_join( migrating_player )
end

function on_client_disconnect( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        local server_id = player.server_id_
        local player_id = player.player_id_
        player_mng.del_player( server_id, player_id )
        bfsvr.remote_call_game( server_id, "bfclient.on_migrated_player_disconnect", player.account_id_, player_id )
        c_log( "[gamesvr](on_client_disconnect) player disconnect, server_id: %d, player_id: %d", server_id, player_id )
    end
    
    local dpid2migrating_players = bfsvr.g_dpid2migrating_players
    local migrating_player = dpid2migrating_players[_dpid]
    if migrating_player then
        migrating_player.dpid_ = nil
        dpid2migrating_players[_dpid] = nil
    end
    player_mng.del_player_by_dpid( _dpid )
    g_dpid_mng[_dpid] = nil
end

function on_rejoin_begin_succ( _ar, _dpid, _size )
    bfsvr.on_player_send_rejoin_begin_succ( _dpid )
end

function on_event_ares_quit( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_ares_bf.player_leave_game( player )
    end
end

function on_ares_witness_quit_witness( _ar, _dpid, _size )
    local match_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_ares_bf.on_witness_player_quit( player, match_sn )
    end
end

function on_ares_witness_like( _ar, _dpid, _size )
    local match_sn = _ar:read_int()
    local like_index = _ar:read_ubyte()
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_ares_bf.on_witness_player_like( player, match_sn, like_index, server_id, player_id )
    end
end

function on_honor_witness_quit_witness( _ar, _dpid, _size )
    local match_sn = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_ladder_bf.on_witness_player_quit( player, match_sn )
    end
end

function on_honor_witness_like( _ar, _dpid, _size )
    local match_sn = _ar:read_int()
    local like_index = _ar:read_ubyte()
    local server_id = _ar:read_int()
    local player_id = _ar:read_int()
    
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_ladder_bf.on_witness_player_like( player, match_sn, like_index, server_id, player_id )
    end
end

function on_event_ladder_quit( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        honor_ladder_bf.on_ladder_player_leave_game( player )
    end
end

function on_req_honor_ladder_use_model( _ar, _dpid, _size )
    local model_index = _ar:read_int()
 
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        return 
    end
    honor_ladder_bf.on_ladder_player_use_model( player, model_index )
end 

function on_auto_join_wild( _ar, _dpid, _size )
    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err("[gamesvr](on_auto_join_wild) can not found player by dpid: 0x%X", _dpid )
        return
    end
    local wild_type = _ar:read_int()
    local trans_id = _ar:read_int()

    wild_mng.on_auto_wild_transfer( player, wild_type, trans_id )
end

function on_event_team_ladder_select_model( _ar, _dpid, _size )
    local is_ladder_match = _ar:read_ubyte()
    local game_count = _ar:read_int()
    local model_idx = _ar:read_int()
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        if is_ladder_match > 0 then
            event_team_ladder.on_player_select_model( player, game_count, model_idx )
        else
            event_team_champion.on_player_select_model( player, game_count, model_idx )
        end
    end
end

function on_event_champion_request_add_like( _ar, _dpid, _size )
    local zone_id = _ar:read_int()
    local round = _ar:read_ubyte()
    local match_rec_id = _ar:read_int()
    local player_number = _ar:read_ubyte() 
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        event_team_champion.on_player_request_add_like( player, zone_id, round, match_rec_id, player_number )
    end
end

func_table = func_table or {}
func_table[msg.PT_ASK_BF_REGISTER_DPID] = on_ask_bf_register_dpid
func_table[msg.PT_DISCONNECT] = on_client_disconnect
func_table[msg.PT_SEND_REJOIN_BEGIN_SUCC] = on_rejoin_begin_succ
func_table[msg.PT_ARES_QUIT_BF] = on_event_ares_quit
func_table[msg.PT_ARES_WITNESS_QUIT_WITNESS] = on_ares_witness_quit_witness
func_table[msg.PT_ARES_WITNESS_LIKE] = on_ares_witness_like
func_table[msg.PT_HONOR_WITNESS_QUIT_WITNESS] = on_honor_witness_quit_witness
func_table[msg.PT_HONOR_WITNESS_LIKE] = on_honor_witness_like
func_table[msg.PT_REQ_HONOR_LADDER_QUIT_BF] = on_event_ladder_quit
func_table[msg.PT_REQ_HONOR_LADDER_USE_MODEL] = on_req_honor_ladder_use_model
func_table[msg.PT_AUTO_JOIN_WILD] = on_auto_join_wild
func_table[msg.PT_TEAM_LADDER_SELECT_MODEL] = on_event_team_ladder_select_model
func_table[msg.PT_TEAM_CHAMPION_REQUEST_ADD_LIKE] = on_event_champion_request_add_like

