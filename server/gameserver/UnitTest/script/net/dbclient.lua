module( "dbclient", package.seeall )

local sformat, mrandom, unpack, slen = string.format, math.random, unpack, string.len
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type
local tinsert = table.insert

g_ar = g_ar or LAr:c_new()
g_max_gm_global_mail_id = g_max_gm_global_mail_id or -1

--rpc分包相关
g_split_rpc_data = g_split_rpc_data or {} 
g_split_rpc_id = g_split_rpc_id or 0
g_best_size_packet_func_name = g_best_size_packet_func_name or {m_size_ = 0, func_name_ = ""}

local function remote_call_db_get_system_global_data()
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_TEAM_LADDER, event_team_ladder.SEASON_DATA_ID )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_ANNOUNCE, announce_mng.DATA_ID_ANNOUNCE_ACTIVITY )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_ANNOUNCE, announce_mng.DATA_ID_ANNOUNCE_CHAT )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_ANNOUNCE, announce_mng.DATA_ID_ANNOUNCE_REBOOT )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_HONOR_LADDER, share_const.GLOBAL_TBL_SYS_ID_HONOR_LADDER_SUB_DAILY_RANK_VERSION )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_TEAM_CHAMPION, event_team_champion.NEXT_MATCH_INFO_ID )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_TEAM_CHAMPION, event_team_champion.LAST_ENROLL_TEAM_INFO_ID )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_TEAM_CHAMPION, event_team_champion.LAST_MATCH_RECORD_ID )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_ONE_GOLD_LUCKY_DRAW, 0 )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_ONE_GOLD_LUCKY_DRAW, 1 )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_ONE_GOLD_LUCKY_DRAW, 2 )
    remote_call_db( "db_system_global_data.get_system_global_data", share_const.GLOBAL_TBL_SYS_ID_GM_ACTIVITY, activity_mng.DATA_ID_GM_ACTIVITY_AVATAR_LOTTERY )  --变身抽奖
end

function on_db_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()

    -- 记录最大size
    if g_best_size_packet_func_name.m_size_ < _size then
        c_safe("on_db_remote_call bigger_size = %s, func_name:%s", _size, func_name)
        g_best_size_packet_func_name.m_size_ = _size
        g_best_size_packet_func_name.func_name_ = func_name
    end

    -- 记录size大于26w
    if _size > 260000 then
        c_safe("on_db_remote_call size > 260000, _size = %s, func_name:%s", _size, func_name)
    end

    local nt = os.date( "*t" )
    if nt.min == 0 or nt.min == 30 then
        c_safe("on_db_remote_call time check size = %s, func_name:%s", _size, func_name)
    end

    local args = _ar:read_buffer()

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "[dbclient](on_db_remote_call)func %s not found", func_name ) )
        return 
    end
    local args_table = c_msgunpack( args )
    func( unpack( args_table ) )
end

function on_db_split_remote_call( _ar, _dpid, _size )
    local split_rpc_id = _ar:read_double()
    local sub_data = _ar:read_buffer()

    local data_list = g_split_rpc_data[split_rpc_id] or {}
    g_split_rpc_data[split_rpc_id] = data_list
    tinsert(data_list, sub_data)
end

function on_db_split_remote_call_end( _ar, _dpid, _size )
    local split_rpc_id = _ar:read_double()
    local func_name = _ar:read_string()

    local data_list = g_split_rpc_data[split_rpc_id] or {}
    g_split_rpc_data[split_rpc_id] = nil 

    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "[dbclient](on_db_remote_call)func %s not found", func_name ) )
        return 
    end

    local data = table.concat(data_list, "") 
    local args_table = c_msgunpack( data )
    func( unpack( args_table ) )
end

--
-- game和db连接上后，向db申请load全局数据的请求都放在这里
-- 注意：remote_call_db( "dbserver.on_get_final_global_data" )必须放在最后
--
function on_db_connected( _ar, _dpid, _size )
    remote_call_db( "dbserver.on_get_honor_hall_global" )
    remote_call_db( "dbserver.on_get_mystic_global" )
    remote_call_db( "dbserver.on_get_recharge_global_data" )
    remote_call_db( "db_honor_hall.query_all_honor_ladder_data" )
    remote_call_db( "db_rank.init_self_server_rank_list" )
    remote_call_db( "db_rank.init_all_server_rank_list" )
--    activity_mng.init_load_resource_magnate_rank_from_db()
    shop_mng.try_query_all_shops()
    gold_exchange_mng.try_query_all_gold_exchanges()
    time_event.try_query_database()
    remote_call_db( "db_mail.on_game_query_max_gm_global_mail_id" )
    remote_call_db( "db_guild_system.do_get_all_guilds" )
    remote_call_db( "db_guild_daily_pvp.get_all_guild_data" )
    remote_call_db( "db_attend.get_attend_push_data" )
    remote_call_db( "db_ares_match.get_newest_ares_matches" )
    remote_call_db( "db_honor_match.get_newest_honor_matches" )
    --remote_call_db( "db_guild_weekend_battle.get_all_guild_data" )
    remote_call_db( "db_ladder_team.get_all_team_data" )
    remote_call_db( "db_player_team_ladder.get_all_player_team_ladder_data" )
    remote_call_db_get_system_global_data()
    auction_mng.sync_item_list_to_bf()
    trade_mng.sync_item_list_to_bf()
    remote_call_db( "db_tower.init_tower_rank_list" )
    remote_call_db( "db_palace.init_palace_rank_list" )
    remote_call_db( "db_arena.do_get_all_arena_player" )
    remote_call_db( "db_activity.do_get_all_activity_param" )
    remote_call_db( "db_activity.do_get_all_activity_param_log" )
--    activity_mng.load_server_group_buy()
    hall_mng.on_db_connected()


    -- |
    -- |最后一条，控制gs端口在之前的数据取回后才开始Listen
    -- V
    remote_call_db( "dbserver.on_get_final_global_data" ) 
end

function set_max_gm_global_mail_id( _max_gm_global_mail_id )
    g_max_gm_global_mail_id = _max_gm_global_mail_id
    c_log( "[dbclient](set_max_gm_global_mail_id) g_max_gm_global_mail_id is set to %d", _max_gm_global_mail_id )
end

--
-- dbserver加载完所有全局数据后，会调用dbclient.on_db_get_all_global_data函数
-- 此刻，玩家才可以进入游戏
--
function on_db_get_all_global_data()
    c_assert( g_gamesvr:c_start_server(), "failed to start server for player, please check" )
    c_log( "[dbclient](on_db_get_all_global_data)all global data get success" ) 
end

function on_db_get_honor_hall_top_player_info( _top_player )
    honor_hall.g_honor_hall_top_player = _top_player   
end 

function on_db_certify_result( _ar, _dpid, _size )
    local dpid = _ar:read_ulong()
    local account_id = _ar:read_ulong()
    local retcode = _ar:read_ushort()
    gamesvr.send_certify_code_to_client( dpid, retcode )
end

local function add_rand_key( _account_info )
    local rand_key = mrandom(2147483647) -- 2^31 - 1
    _account_info.rand_key_ = rand_key
    return _account_info
end

function on_db_get_account_info( _dpid, _account_info )
    local dpid_info = gamesvr.g_dpid_mng[_dpid]
    if dpid_info then
        dpid_info.account_info_ = add_rand_key( _account_info )
        gamesvr.g_account_name_2_dpid[_account_info.account_name_] = _dpid
        gamesvr.send_player_list_to_client( _dpid, _account_info )
        c_login( "[dbclient](on_db_get_account_info) dpid: 0x%X, account_id: %d, rand_key: %d"
               , _dpid, _account_info.account_id_, _account_info.rand_key_ )
    else
        c_log( "[dbclient](on_db_get_account_info) dpid_info not found, dpid: 0x%X", _dpid )
    end
end

function on_db_get_random_name( _dpid, _player_name )
    gamesvr.send_random_name_to_client( _dpid, _player_name )
end

function on_db_get_mystic_global( _dealer_info )
    mystic_dealer.init_mystic_global_from_str( _dealer_info )
end

function on_db_get_honor_hall_global_info( _info )
    honor_hall.honor_hall_init_global_info_from_str( _info )
end

function on_db_get_recharge_global_data( _uncompleted_orders )
    order_mng.load_uncompleted_orders( _uncompleted_orders )
end

function on_db_get_recharge_global_data_completed()
    order_mng.start_query_batch_timer()
end

function on_db_create_player( _dpid, _retcode, _player_id, _player_name )
    gamesvr.send_create_player_result_to_client( _dpid, _retcode, _player_id, _player_name )
end

function on_db_delete_player( _dpid, _retcode, _player_id, _account_info)
    if _retcode == login_code.DELETE_PLAYER_OK then 
        local dpid_info = gamesvr.g_dpid_mng[_dpid]
        if dpid_info then
            dpid_info.account_info_ = add_rand_key( _account_info )
            gamesvr.g_account_name_2_dpid[_account_info.account_name_] = _dpid
            gamesvr.send_player_list_to_client( _dpid, _account_info)
            c_log( "[dbclient](on_db_delete_player) delete player success dpid: 0x%X account_id:%.0f player_id:%d", _dpid, _account_info.account_name_, _player_id)
        else
            c_log( "[dbclient](on_db_delete_player) dpid_info not found, dpid: 0x%X", _dpid )
        end
        player_mng.on_player_del_in_db(_player_id)
    else
        gamesvr.send_delete_player_result_to_client( _dpid, _retcode )
        c_err( "[dbclient](on_db_delete_player) delete player fail dpid: 0x%X player_id:%d retcode:%d", _dpid, _player_id, _retcode)
    end
end

function on_db_send_merge_offline_mail_quest( _player_id, _origin_mails_str, _new_mails_str )
    mail_box_t.on_merge_offline_mail_quest( _player_id, _origin_mails_str, _new_mails_str )
end

function on_db_player_join( _dpid, _retcode, _player )
    if _player then
        local player_id = _player.player_id_
        local joining_player = gamesvr.g_joining_players[player_id]
        if not joining_player then
            c_log( "[dbclient](on_db_player_join) dpid: 0x%X, player_id: %d, no such joining player found, the player may have disconnected", _dpid, player_id )
            return
        end
        joining_player.player_from_db_ = _player
        gamesvr.do_join_on_recv_from_peer( joining_player )
        c_log( "[dbclient](on_db_player_join) dpid: 0x%X, player_id: %d", _dpid, player_id )
    else
        gamesvr.send_player_join_result_to_client( _dpid, _retcode )
        c_err( "[dbclient](on_db_player_join) dpid: 0x%X, player not found, check db for details", _dpid )
    end
end

function on_player_rename_check_result( _player_id, _player_name, _result_code )
    local player = player_mng.get_player_by_player_id( _player_id )
    if not player then return end

    if _result_code ~= login_code.PLAYER_NAME_OK then
        player:add_player_rename_result( _result_code )
        return
    end

    local PLAYER_RENAME_DIAMOND = 188
    if not player:try_consume_diamond( PLAYER_RENAME_DIAMOND ) then
        remote_call_db( "db_login.unlock_name_of_account", player.account_id_ )
        c_err( "[dbclient](on_player_rename_check_result) try_consume_diamond %d fail, player_id: %d", PLAYER_RENAME_DIAMOND, _player_id )
        return
    end
    player:consume_diamond( PLAYER_RENAME_DIAMOND, "[PLAYER_RENAME]" )
    local old_player_name = player.player_name_
    player.player_name_ = _player_name  -- 先改game内存中的名字，不然存盘时可能会旧名覆盖新名
    player:to_string()
    remote_call_db( "db_login.do_player_rename", player.account_id_, _player_id, _player_name, player.bind_diamond_, player.non_bind_diamond_ )
    player_mng.on_player_rename( player, old_player_name )
    player:syn_team_member_info( true )
    player:on_ladder_team_member_change_name()
    honor_ladder_game.on_player_name_change( player )
    tower_rank_mng.on_player_rename( player )
    palace_rank_mng.on_player_rename( player )
    c_log( "[dbclient](on_player_rename_check_result) player_id: %d, "
         .."old player_name: %s, new player_name: %s", _player_id, old_player_name, _player_name
         )
end

function on_player_rename_result( _player_id, _player_name, _result_code )
    local player = player_mng.get_player_by_player_id( _player_id )
    if not player then return end
    if _result_code == login_code.PLAYER_NAME_OK then
        player_mng.broadcast_player_rename_result( _player_id, _result_code )
    else
        player:add_player_rename_result( _result_code )
    end
end

function on_get_account_ids_to_forbid_accounts( _browser_client_id, _account_ids, _timestamps, _is_forbid_machine )
    loginclient.remote_call_ls( "game_login_server.forbid_accounts", _browser_client_id, _account_ids, _timestamps, _is_forbid_machine  )
end

func_table =
{
    [msg.PT_CONNECT] = on_db_connected,
    [msg.DB_TYPE_RPC] = on_db_remote_call,
    [msg.DB_TYPE_SPLIT_RPC] = on_db_split_remote_call,
    [msg.DB_TYPE_SPLIT_RPC_END] = on_db_split_remote_call_end,
    [msg.DB_TYPE_CERTIFY_RESULT] = on_db_certify_result,
}

function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        c_err( "[LUA_MSG_HANDLE](dbclient) unknown packet 0x%08x %08x", _packet_type, _dpid )
    end  
end

function send_exec_sql( _sql )
    local ar = g_ar
    ar:flush_before_send( msg.DB_TYPE_EXEC_SQL )
    ar:write_string( _sql )
    ar:send_one_ar( g_dbclient, 0 )
end

function remote_call_db( _func_name, ... )
    local ar = g_ar
    local arg = {...}    
    ar:flush_before_send( msg.DB_TYPE_RPC )
    ar:write_string( _func_name )
    ar:write_buffer( c_msgpack( arg ) )
    ar:send_one_ar( g_dbclient, 0 )
end

function split_remote_call_db(_func_name, ...)
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

        ar:flush_before_send( msg.DB_TYPE_SPLIT_RPC )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_dbclient, 0 )
    end

    if total_len > count * unit_len then
        local begin_idx = 1 + count * unit_len
        local end_idx = total_len 
        local sub_data = string.sub(data, begin_idx, end_idx)  

        ar:flush_before_send( msg.DB_TYPE_SPLIT_RPC )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_dbclient, 0 )
    end

    ar:flush_before_send( msg.DB_TYPE_SPLIT_RPC_END )
    ar:write_double( split_rpc_id )
    ar:write_string( _func_name )
    ar:send_one_ar( g_dbclient, 0 )
end

function send_certify_to_db( _dpid, _account_id )

    local client_ip_str = g_gamesvr:c_get_player_ip( _dpid )

    local ar = g_ar         
    ar:flush_before_send( msg.DB_TYPE_CERTIFY )
    ar:write_ulong( _dpid )
    ar:write_double( _account_id )
    ar:write_string(client_ip_str)
    ar:send_one_ar( g_dbclient, 0 )
end

function send_get_playerlist_to_db( _dpid, _account_id )
    local ar = g_ar         
    ar:flush_before_send( msg.DB_TYPE_GET_PLAYERLIST )
    ar:write_ulong( _dpid )
    ar:write_ulong( _account_id )
    ar:send_one_ar( g_dbclient, 0 )
end

function send_get_random_name_to_db( _dpid, _gender )
    local account_id = login_mng.get_account_id_by_dpid( _dpid )
    if account_id then
        remote_call_db( "dbserver.on_get_random_name", _dpid, account_id, _gender )
    else
        c_err( "[dbclient](send_get_random_name_to_db) cannot get account_id by dpid: 0x%X", _dpid )
    end
end

function send_unlock_name_to_db( _dpid )
    local account_id = login_mng.get_account_id_by_dpid( _dpid )
    if account_id then
        remote_call_db( "db_login.unlock_name_of_account", account_id )
    else
        c_err( "[dbclient](send_unlock_random_name_to_db) cannot get account_id by dpid: 0x%X", _dpid )        
    end
end

function send_create_player_to_db( _dpid, _msglist )
    remote_call_db( "dbserver.on_create_player", _dpid, _msglist )
end

function send_delete_player_to_db( _dpid, _account_id, _player_id )
	remote_call_db( "dbserver.on_delete_player", _dpid, _account_id, _player_id )
end

function send_join_to_db( _dpid, _player_id, _log_info )
    remote_call_db( "dbserver.on_player_join", _dpid, _player_id, _log_info )
end

function send_save_player_to_db( _online, _player)
    local player_id = _player.player_id_
    _player:update_game_time_before_save( _online )
    honor_hall.honor_hall_save_player_score_to_db( player_id )
    local ar = g_ar         
    ar:flush_before_send( msg.DB_TYPE_SAVE_PLAYER )
    ar:write_boolean( _online )
    _player:serialize_to_peer( ar, true )
    ar:send_one_ar( g_dbclient, 0 )
    c_log( "[dbclient](send_save_player_to_db) online: %d, player_id: %d", _online and 1 or 0, player_id )
end

function send_save_player_buff_to_db( _player )
end

function send_set_account_authority_to_db( _account_name, _authority )
    remote_call_db( "dbserver.on_set_account_authority", _account_name, _authority )
end

function send_save_player_offline_mails_to_db( _player_id, _need_merge, _mails_str )
    remote_call_db( "dbserver.on_save_player_offline_mails", _player_id, _need_merge, _mails_str )
end

function send_session_closed_to_db( _account_id )
    remote_call_db( "dbserver.on_client_session_closed", _account_id )
end

function send_save_mystic_global_to_db(  _dealer_info_str )
    remote_call_db( "dbserver.on_save_mystic_global", _dealer_info_str )
end

function send_save_honor_hall_global_to_db( _str )
    remote_call_db( "db_honor_hall.do_save_honor_hall_global_info", _str )
end

local function write_shop_items( _ar, _shop )
    _ar:write_int( _shop.shop_local_id_ )
    _ar:write_string( shop_t.serialize_shop_items_to_peer( _shop ) )
    _ar:write_double( _shop.next_item_sn_in_shop_ )
end

function send_save_shop_items_to_db( _shop )
    local ar = g_ar
    ar:flush_before_send( msg.DB_TYPE_SAVE_SHOP_ITEMS )
    write_shop_items( ar, _shop )
    ar:send_one_ar( g_dbclient, 0 )
end

function send_save_bag_shop_items_to_db( _shop )
    local player = player_mng.get_player_by_player_id( _shop.player_id_ )
    local ar = g_ar
    ar:flush_before_send( msg.DB_TYPE_SAVE_BAG_SHOP_ITEMS )
    ar:write_boolean( player ~= nil )
    if player then
        ar:write_int( player.player_id_ )
        player.bag_:serialize_bag_to_peer( ar )
    end
    write_shop_items( ar, _shop )
    ar:send_one_ar( g_dbclient, 0 )
end

function send_save_shop_to_db_on_sell( _params )
    local ar = g_ar
    ar:flush_before_send( msg.DB_TYPE_SAVE_SHOP_ON_SELL )
    ar:write_int( _params.shop_local_id_ )
    ar:write_int( _params.gold_ )
    ar:write_int( _params.bind_gold_ )
    ar:write_string( _params.deal_history_str_ )
    ar:write_string( _params.items_str_ )
    ar:write_string( _params.sold_item_str_ )
    ar:write_int( _params.buyer_server_id_ )
    ar:write_int( _params.buyer_player_id_ )
    ar:send_one_ar( g_dbclient, 0 )
end

function send_save_shop_to_db_on_maintain( _shop_local_id, _gold, _pay_tax_time, _deal_history_str )
    remote_call_db( "db_shop.save_shop_on_maintain", _shop_local_id, _gold, _pay_tax_time, _deal_history_str )
end

function send_delete_shop( _shop_local_id, _player_id )
    remote_call_db( "db_shop.delete_shop", _shop_local_id, _player_id )
end

function on_gm_query_player_info( _browser_client_id, _player )
    loginclient.remote_call_ls( "game_login_server.on_gm_query_player_info", _browser_client_id, _player )
end

function on_gmsvr_query_player_info( _dpid, _player )
    loginclient.remote_call_ls( "game_login_server.on_gmsvr_query_player_info", _dpid, _player )
end

function on_get_system_global_data( _system_id, _sub_id, _data_str )
    if _system_id == share_const.GLOBAL_TBL_SYS_ID_TEAM_LADDER then
        event_team_ladder.on_get_global_data_from_db( _sub_id, _data_str )
    elseif _system_id == share_const.GLOBAL_TBL_SYS_ID_ANNOUNCE then
        announce_mng.init_data(_sub_id, _data_str)
    elseif _system_id == share_const.GLOBAL_TBL_SYS_ID_HONOR_LADDER then
        honor_ladder_game.on_honor_ladder_global_data_from_db( _sub_id, _data_str )
    elseif _system_id == share_const.GLOBAL_TBL_SYS_ID_TEAM_CHAMPION then
        event_team_champion.on_get_global_data_from_db( _sub_id, _data_str )
    elseif _system_id == share_const.GLOBAL_TBL_SYS_ID_ONE_GOLD_LUCKY_DRAW then
        event_one_gold_lucky_draw.on_get_data_from_db( _sub_id, _data_str )
    elseif _system_id == share_const.GLOBAL_TBL_SYS_ID_GM_ACTIVITY then
        activity_mng.on_get_data_from_db( _sub_id, _data_str )
    end
end

function send_online_player_num_to_db( _online_num )
    local ar = g_ar
    ar:flush_before_send( msg.DB_TYPE_SAVE_ONLINE_PLAYER_NUM )
    ar:write_int( os.time() )
    ar:write_int( _online_num )

    ar:send_one_ar( g_dbclient, 0 )
end

