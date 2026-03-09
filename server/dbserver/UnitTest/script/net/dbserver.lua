module( "dbserver", package.seeall )

MAX_DATABASE_STR_LEN = 1024

--------------------------------------------------
-- global var
--------------------------------------------------
local sformat, unpack, tinsert, slen = string.format, unpack, table.insert, string.len
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

g_ar = g_ar or LAr:c_new()
g_db_character = g_db_character or DataBase:c_new()
g_game_dpid = g_game_dpid or 0
g_frame = g_frame or 0

--rpc分包相关
g_split_rpc_data = g_split_rpc_data or {} 
g_split_rpc_id = g_split_rpc_id or 0

--------------------------------------------------
-- db server functions
--------------------------------------------------
if not g_init then
    g_init = function ()
        local host, user, passwd, port, dbname = c_read_mysql( "sgame_character" )
        if( g_db_character:c_connect( host, user, passwd, port ) < 0 ) then
            c_err( "[dbserver] (init) connect database test failed.".. "[" .. host .. ":" .. port .."]".. user )
            return false
        end
        if g_db_character:c_select_db( dbname ) ~= 0 then
            c_err( "[dbserver] (init) select database test failed" )
            return false
        end
        return true 
    end
    assert( g_init() )
end

function on_exec_sql( _ar, _dpid, _size )
    local sql = _ar:read_string()
    return exec_sql(sql)
end

function exec_sql(sql)
    if (sql == nil) then
        c_err( "sql is nil" )
        return false
    end
    if( g_db_character:c_query(sql) < 0 ) then
        c_err( "failed to exec sql:"..sql )
        return false
    end
    local rows = g_db_character:c_row_num()
    for i=1, rows do
        if( g_db_character:c_fetch() == 0 ) then
        end
    end
end

function on_db_remote_call( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args = _ar:read_buffer()
    local func = loadstring( sformat( "return _G.%s", func_name ) )()
    if not func then 
        c_err( sformat( "[dbserver](on_db_remote_call)func %s not found", func_name ) )
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

function on_certify( _ar, _dpid, _size )
    local dpid = _ar:read_ulong()
    local account_id = _ar:read_double()
    local client_ip_str = _ar:read_string()
    local retcode = db_login.do_certify( account_id, client_ip_str )
    if retcode == login_code.CERTIFY_OK then
        local account_info = db_login.do_get_account_info( account_id )
        remote_call_game( "dbclient.on_db_get_account_info", dpid, account_info )
    else
        send_certify_result( dpid, account_id, retcode )
    end
end

function on_get_player_list( _ar, _dpid, _size )
    local dpid = _ar:read_ulong()
    local account_id = _ar:read_ulong()
    local account_info = db_login.do_get_account_info( account_id )
    remote_call_game( "dbclient.on_db_get_account_info", dpid, account_info )
end

function on_get_random_name( _dpid, _account_id, _gender )
    local player_name = db_random_names.get_random_name( _account_id, _gender )
    remote_call_game( "dbclient.on_db_get_random_name", _dpid, player_name )
end

function send_merge_offline_mail_quest_to_game( _player_id, _origin_mails_str, _new_mails_str )
    remote_call_game( "dbclient.on_db_send_merge_offline_mail_quest", _player_id, _origin_mails_str, _new_mails_str )
end

function on_create_player( _dpid, _msg_list )
    local retcode, player_id, player_name = db_login.do_create_player( _dpid, _msg_list )
    if retcode == login_code.CREATE_PLAYER_OK then
        local account_info = db_login.do_get_account_info( _msg_list.account_id_ )
        remote_call_game( "dbclient.on_db_get_account_info", _dpid, account_info )
    end
    remote_call_game( "dbclient.on_db_create_player", _dpid, retcode, player_id or 0, player_name or "" )
end

function on_delete_player( _dpid, _account_id, _player_id )
    local retcode = db_login.do_delete_player( _account_id, _player_id )
    local account_info = db_login.do_get_account_info( _account_id )
    remote_call_game( "dbclient.on_db_delete_player", _dpid, retcode, _player_id, account_info )
end

function on_player_join( _dpid, _player_id, _log_info )
    local retcode, player = db_login.do_player_join( _player_id, _log_info )
    remote_call_game( "dbclient.on_db_player_join", _dpid, retcode, player )
end

function on_save_player( _ar, _dpid, _size )
    local online = _ar:read_boolean()
    local player = player_t.deserialize_from_peer( _ar )
    db_login.do_save_player( online, player )
end

function on_save_shop_items( _ar, _dpid, _size )
    db_shop.save_shop_items_by_ar( _ar )
end

function on_save_bag_shop_items( _ar, _dpid, _size )
    local online = _ar:read_boolean()
    if online then
        local player_id = _ar:read_int()
        db_bag.save_player_bag_by_ar( player_id, _ar )
    end
    db_shop.save_shop_items_by_ar( _ar )
end

function on_save_shop_on_sell( _ar, _dpid, _size )
    local shop_local_id = _ar:read_int()
    db_shop.save_shop_by_ar_on_sell( shop_local_id, _ar )
    local item_str = _ar:read_string()
    local buyer_server_id = _ar:read_int()
    local buyer_player_id = _ar:read_int()
    remote_call_game( "shop_mng.give_item_to_buyer_from_db", shop_local_id, item_str, buyer_server_id, buyer_player_id )
    c_log( "[dbserver](on_save_shop_on_sell) shop_local_id: %d, buyer_server_id: %d, buyer_player_id: %d, item_str: %s"
         , shop_local_id, buyer_server_id, buyer_player_id, item_str
         )
end

function on_save_player_offline_mails( _player_id, _need_merge, _mails_str )
    db_mail.save_player_offline_mails( _player_id, _need_merge, _mails_str )
end

function on_set_account_authority( _account_name, _authority )
    db_login.set_account_authority( _account_name, _authority )
end

function on_client_session_closed( _account_id )
    db_login.unlock_name_of_account( _account_id )
end

function on_get_mystic_global( )
    local mystic_global_info = db_mystic.do_get_mystic_global()
    remote_call_game( "dbclient.on_db_get_mystic_global", mystic_global_info )
end

function on_get_honor_hall_global( )
    local honor_hall_global_info = db_honor_hall.do_get_honor_hall_global_info()
    remote_call_game( "dbclient.on_db_get_honor_hall_global_info", honor_hall_global_info )
end

function on_get_recharge_global_data()
    local uncompleted_orders = db_order.load_uncompleted_orders()

    local split_list = {}
    local count = 0

    for _, order_info in ipairs( uncompleted_orders ) do
        if count == 0 then
            tinsert( split_list, {} )
        end
        local list = split_list[#split_list]
        tinsert( list, order_info )
        count = count + 1
        if count == 100 then
            count = 0
        end
    end

    for _, split_orders in ipairs( split_list ) do
        remote_call_game( "dbclient.on_db_get_recharge_global_data", split_orders )
    end

    remote_call_game( "dbclient.on_db_get_recharge_global_data_completed" )

    c_sells( "[dbserver](on_get_recharge_global_data)uncompleted orders num: %d, load completed", #uncompleted_orders );
end

function on_get_final_global_data()
    remote_call_game( "dbclient.on_db_get_all_global_data" )
end

function on_save_mystic_global( _dealer_info_str )
    db_mystic.do_save_mystic_global( _dealer_info_str )
end

function on_game_disconnect( _ar, _dpid, _size )
    if _dpid == g_game_dpid then
        db_login.unlock_all_names()
        c_log( "[dbserver](on_game_disconnect) game disconnected" )
    end
end

function on_get_player_snapshot(_player_id, _call_back, ...)
    local player = db_login.get_player_snapshot(_player_id) or {}
    dbserver.remote_call_game(_call_back, player, ...)
end

function on_batch_get_player_snapshot(_player_list, _call_back, ...)
    local snapshot_list = db_login.batch_get_player_snapshot(_player_list)
    if snapshot_list ~= nil then
        dbserver.remote_call_game(_call_back, snapshot_list, ...)
    end
end

function on_load_player_info(_player_id, _func_name_callback, _param)
    local retcode, player = db_login.do_player_join(_player_id)
    remote_call_game( "arena_mng.on_db_load_player_info", retcode, player, _func_name_callback, _param)
end

function on_get_player_orders(_call_back, _param)
    local orders = db_order.get_player_orders(_param.player_id_)
    remote_call_game(_call_back, _param, orders)
end

function on_get_item_log(_call_back, _param)
    local result = {}
    result.count_, result.item_log_list_ = db_item_log.do_get_player_item_log(_param.player_id_, _param.item_id_, _param.begin_time_, _param.end_time_)
    if not result.item_log_list_ then
        c_err("call db error")
        return
    end
    remote_call_game(_call_back, _param, result)
end


func_table =
{
    [msg.DB_TYPE_RPC]                        = on_db_remote_call,
    [msg.DB_TYPE_SPLIT_RPC]                  = on_db_split_remote_call,
    [msg.DB_TYPE_SPLIT_RPC_END]              = on_db_split_remote_call_end,
    [msg.DB_TYPE_EXEC_SQL]                   = on_exec_sql,
    [msg.DB_TYPE_CERTIFY]                    = on_certify,
    [msg.DB_TYPE_GET_PLAYERLIST]             = on_get_player_list,
    [msg.DB_TYPE_SAVE_PLAYER]                = on_save_player,
    [msg.DB_TYPE_SAVE_BAG_SHOP_ITEMS]        = on_save_bag_shop_items,
    [msg.DB_TYPE_SAVE_SHOP_ON_SELL]          = on_save_shop_on_sell,
    [msg.DB_TYPE_SAVE_SHOP_ITEMS]            = on_save_shop_items,
    [msg.PT_CONNECT]                         = on_game_connect,
    [msg.PT_DISCONNECT]                      = on_game_disconnect,
}

function remote_call_game( _func_name, ... )
    local ar = g_ar
    local arg = {...}    
    ar:flush_before_send( msg.DB_TYPE_RPC )
    ar:write_string( _func_name ) 
    local data = c_msgpack( arg )
    local size = slen(data)
    if size > 260000 then
        c_safe("remote_call_game too big packet func_name:%s size:%d", _func_name, size)
    end
    ar:write_buffer( data )
    ar:send_one_ar( g_dbsvr, g_game_dpid )
end

function split_remote_call_game(_func_name, ...)
    local ar = g_ar
    local arg = {...}    
    local data = c_msgpack( arg )
    local split_rpc_id = g_split_rpc_id 
    g_split_rpc_id = g_split_rpc_id + 1
     
    local total_len = slen(data) 
    local unit_len = utils.SPLIT_PACKET_LEN
    local count = math.floor(total_len / unit_len)
    for i=1, count do 
        local begin_idx = 1 + (i-1 ) * unit_len
        local end_idx = i * unit_len
        local sub_data = string.sub(data, begin_idx, end_idx)  

        ar:flush_before_send( msg.DB_TYPE_SPLIT_RPC )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_dbsvr, g_game_dpid )
    end

    if total_len > count * unit_len then
        local begin_idx = 1 + count * unit_len
        local end_idx = total_len 
        local sub_data = string.sub(data, begin_idx, end_idx)  

        ar:flush_before_send( msg.DB_TYPE_SPLIT_RPC )
        ar:write_double( split_rpc_id )
        ar:write_buffer( sub_data )
        ar:send_one_ar( g_dbsvr, g_game_dpid )
    end

    ar:flush_before_send( msg.DB_TYPE_SPLIT_RPC_END )
    ar:write_double( split_rpc_id )
    ar:write_string( _func_name )
    ar:send_one_ar( g_dbsvr, g_game_dpid )
end

--------------------------------------------------
-- callback functions for c/c++
--------------------------------------------------

function on_exit()
    if (g_db_character) then
        g_db_character:c_delete()
    end
end

function set_game_dpid( _dpid, _connected )
    g_game_dpid = _dpid
end

function set_frame( _frame )
    g_frame = _frame
end

function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        c_err( sformat( "[LUA_MSG_HANDLE](dbserver) unknown packet 0x%08x %08x", _packet_type, _dpid) )
    end  
end

function send_certify_result( _dpid, _account_id, _retcode )
    local ar = g_ar
    ar:flush_before_send( msg.DB_TYPE_CERTIFY_RESULT )
    ar:write_ulong( _dpid )
    ar:write_ulong( _account_id )
    ar:write_ushort( _retcode )
    ar:send_one_ar( g_dbsvr, g_game_dpid )
end
