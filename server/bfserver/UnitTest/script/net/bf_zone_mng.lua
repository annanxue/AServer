module( "bf_zone_mng", package.seeall )
local sformat, unpack, msg, ostime, tinsert = string.format, unpack, msg, os.time, table.insert
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

g_check_gamesvr_cfg_timer_ = g_check_gamesvr_cfg_timer_ or -1
g_gamesvr_cfg_version = 0
g_last_send_version = 0

g_server_list_str = ""
g_zone_list_str = ""
g_server_list = g_server_list or nil
g_zone_list = g_zone_list or nil

g_is_notify = g_is_notify or false

-- cache
g_zone_server_list = g_zone_server_list or nil
g_server_2_zone_idmap = g_server_2_zone_idmap or nil

function check_gamesvr_cfg()
    local base_url = g_bfsvr:c_get_gamesvr_config_url()
    local server_list_url = sformat( "%s%s", base_url, "server_list.ini" )
    local zone_list_url = sformat( "%s%s", base_url, "zone_list.ini" )
    http_mng.request( server_list_url, "", on_server_list_response )
    http_mng.request( zone_list_url, "", on_zone_list_response )
end

function on_server_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[bfsvr](on_server_list_response) check server list failed, response: %s", _response )
        return
    end
    if _response == g_server_list_str then
        c_log( "[bfsvr](on_server_list_response) server list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[bfsvr](on_server_list_response) update server list failed, response: %s", _response )
        return
    end
    g_server_list_str = _response
    g_server_list = func()
    g_gamesvr_cfg_version = ostime()

    -- 修正server_id
    local real_server_list  = {}
    for server_id, server_config in pairs( g_server_list ) do
        local real_server_id = utils.split_number(server_id , 8)
        real_server_list[real_server_id] = server_config
    end 
    g_server_list = real_server_list

    c_log( "[bfsvr](on_server_list_response) update server list succ, version: %d", g_gamesvr_cfg_version )

    if is_load_zone_info_finish() then
        on_load_zone_info_finish()
    end
end

function on_zone_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[bfsvr](on_zone_list_response) check zone list failed, response: %s", _response )
        return
    end
    if _response == g_zone_list_str then
        c_log( "[bfsvr](on_zone_list_response) zone list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[bfsvr](on_zone_list_response) update zone list failed, response: %s", _response )
        return
    end
    g_zone_list_str = _response
    g_zone_list = func()
    g_gamesvr_cfg_version = ostime()
    c_log( "[bfsvr](on_zone_list_response) update zone list succ, version: %d", g_gamesvr_cfg_version )

    if is_load_zone_info_finish() then
        on_load_zone_info_finish()
    end
end

function get_zone_by_server_id(_server_id)
    if not g_server_list then
        c_err("[bf_zone_mng](get_zone_by_server_id) g_server_list is nil")
        return -1
    end
    local svr_cfg = g_server_list[_server_id]
    if not svr_cfg then
        c_err("[bf_zone_mng](get_zone_by_server_id) not find server:%d cfg", _server_id or -1)
        return -1
    end
    
    local svr_zone_id = svr_cfg.ZoneId
    local zone_id = -1
    for id, zone_cfg in pairs(g_zone_list) do
        if svr_zone_id >= zone_cfg.MinZoneInGroup and svr_zone_id <= zone_cfg.MaxZoneInGroup then
            zone_id = id 
            break
        end
    end
    
    if zone_id < 0 then
        c_err("[bf_zone_mng](get_zone_by_server_id) server:%d svr_zone_id:%d not find match zone_id in zone_list", _server_id, svr_zone_id)
        return -1
    end

    return zone_id
end

function get_server_name( _server_id )
    local server_info = g_server_list[ _server_id ]
    if not server_info then 
        return ""
    end 
    
    return server_info["Name"]
end

function get_zone_server_list( _server_id )
    if not g_zone_server_list or not g_server_2_zone_idmap then
        c_err( "[bf_zone_mng](get_zone_server_list) cache not make!" )
        return {}
    end

    local zone_group_id = g_server_2_zone_idmap[ _server_id ] 
    if not zone_group_id then
        c_err( "[bf_zone_mng](get_zone_server_list) zone group id not found by server_id:%d!", _server_id )
        return {}
    end

    local server_list = g_zone_server_list[ zone_group_id ] 
    if not server_list then
        c_err( "[bf_zone_mng](get_zone_server_list) zone group:%d do not contain any server!", zone_group_id )
        return {}
    end
    return server_list
end

function get_zone_server_list_by_zone_id( _zone_id )
    if not g_zone_server_list then
        c_err( "[bf_zone_mng](get_zone_server_list_by_zone_id) cache not make!" )
        return {}
    end 
    
    local server_list = g_zone_server_list[ _zone_id ] 
    if not server_list then
        c_err( "[bf_zone_mng](get_zone_server_list_by_zone_id) zone_id: %d doesn't contain any server.", _zone_id )
        return {}
    end
    return server_list
end

function is_load_zone_info_finish()
    return g_server_list ~= nil and g_zone_list ~= nil 
end

function on_load_zone_info_finish()
    -- make cache
    g_zone_server_list = {}
    g_server_2_zone_idmap = {}
    for server_id, server_config in pairs( g_server_list ) do
        local zone_group_id = get_zone_by_server_id( server_id )
        g_server_2_zone_idmap[ server_id ] = zone_group_id
        
        local my_zone_server_list = g_zone_server_list[ zone_group_id ]
        if not my_zone_server_list then
            my_zone_server_list = {}
            g_zone_server_list[ zone_group_id ] = my_zone_server_list
        end
        tinsert( my_zone_server_list, server_id )
    end

    if g_is_notify then
        return 
    end

    local bf_id =  g_bfsvr:c_get_bf_id()  
    local all_server_id = bfsvr.g_game2dpid_map 
    for svr_id, _ in pairs(all_server_id) do
        bfsvr.remote_call_game( svr_id, "bfclient.on_bf_load_zone_info", bf_id)
    end

    rank_list_mng.on_server_start()
    event_weekend_pvp.clear_zone_last_rank_record()
    trade_mng.on_server_start()
    auction_mng.on_server_start()
    trade_statistic_mng.on_server_start()
    guild_daily_pvp.on_bf_server_start()

    g_is_notify = true
end

function remote_call_group_server( _server_id, _func, _func_name, ... ) 
    if g_debug then
        --函数比较容易改漏，校验一下他们是同一个东西
        local func = loadstring( sformat( "return _G.%s", _func_name ) )()
        if func ~= _func then
            c_err( "[game_zone_mng](remote_call_group_server) func should be the same, name:%s", _func_name )
            c_bt()
            return
        end
    end

    bfsvr.remote_call_game( _server_id, _func_name, ... )
end


function start()
    if g_check_gamesvr_cfg_timer_ ~= -1 then
        g_timermng:c_del_timer( g_check_gamesvr_cfg_timer_ )
    end
    g_check_gamesvr_cfg_timer_ = g_timermng:c_add_timer_second( 0, timers.CHECK_GAMESVR_CFG, 0, 0, 60 )
    --g_check_gamesvr_cfg_timer_ = g_timermng:c_add_timer_second( 0, timers.CHECK_GAMESVR_CFG, 0, 0, 0 )
end

start()
