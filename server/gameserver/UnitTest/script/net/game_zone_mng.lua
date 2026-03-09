module( "game_zone_mng", package.seeall )

local sformat, unpack, msg, ostime, tinsert, tconcat = string.format, unpack, msg, os.time, table.insert, table.concat
local c_msgpack, c_msgunpack, ttype = cmsgpack.pack, cmsgpack.unpack, type

g_check_gamesvr_cfg_timer_ = g_check_gamesvr_cfg_timer_ or -1
g_gamesvr_cfg_version = 0
g_last_send_version = 0

g_server_list_str = ""
g_zone_list_str = ""

g_server_list = g_server_list or nil
g_zone_list = g_zone_list or nil

g_my_zone_group_config = g_my_zone_group_config or nil
g_my_zone_group_server_list = g_my_zone_group_server_list or {}
g_my_server_id = g_my_server_id or -1  -- g_gamesvr:c_get_server_id()
g_my_server_name = g_my_server_name or ""




--------------------------------------------------------------------------
-- Fetch gamesvr_config
--------------------------------------------------------------------------
function start()
    if g_check_gamesvr_cfg_timer_ ~= -1 then
        g_timermng:c_del_timer( g_check_gamesvr_cfg_timer_ )
    end
    local expire_time = g_debug and 0 or 60
    --g_check_gamesvr_cfg_timer_ = g_timermng:c_add_timer_second( 0, timers.CHECK_GAMESVR_CFG, 0, 0, 60 )
    g_check_gamesvr_cfg_timer_ = g_timermng:c_add_timer_second( 0, timers.CHECK_GAMESVR_CFG, 0, 0, expire_time )
end

function check_gamesvr_cfg()
    local base_url = g_gamesvr:c_get_gamesvr_config_url()
    local server_list_url = sformat( "%s%s", base_url, "server_list.ini" )
    local zone_list_url = sformat( "%s%s", base_url, "zone_list.ini" )
    http_mng.request( server_list_url, "", on_server_list_response )
    http_mng.request( zone_list_url, "", on_zone_list_response )
end

function on_server_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[game_zone_mng](on_server_list_response) check server list failed, response: %s", _response )
        return
    end
    if _response == g_server_list_str then
        c_log( "[game_zone_mng](on_server_list_response) server list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[game_zone_mng](on_server_list_response) update server list failed, response: %s", _response )
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


    c_log( "[game_zone_mng](on_server_list_response) update server list succ, version: %d", g_gamesvr_cfg_version )

    if g_server_list and g_zone_list then
        cache_server_list_in_my_zone_group()
        on_load_zone_info_finish()
    end

end

function on_zone_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[game_zone_mng](on_zone_list_response) check zone list failed, response: %s", _response )
        return
    end
    if _response == g_zone_list_str then
        c_log( "[game_zone_mng](on_zone_list_response) zone list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[game_zone_mng](on_zone_list_response) update zone list failed, response: %s", _response )
        return
    end
    g_zone_list_str = _response
    g_zone_list = func()
    g_gamesvr_cfg_version = ostime()
    c_log( "[game_zone_mng](on_zone_list_response) update zone list succ, version: %d", g_gamesvr_cfg_version )

    if g_server_list and g_zone_list then
        cache_server_list_in_my_zone_group()
        on_load_zone_info_finish()
    end
end

function cache_server_list_in_my_zone_group()
    local my_server_id = g_gamesvr:c_get_server_id()
    local svr_cfg = g_server_list[ my_server_id ]
    if not svr_cfg then
        c_err("[game_zone_mng](cache_server_list_in_my_zone_group) not find server:%d cfg", my_server_id)
        return -1
    end
    
    g_my_zone_group_config = nil
    g_my_zone_group_server_list = {}
    local svr_zone_id = svr_cfg.ZoneId
    for id, zone_cfg in pairs(g_zone_list) do
        if svr_zone_id >= zone_cfg.MinZoneInGroup and svr_zone_id <= zone_cfg.MaxZoneInGroup then
            g_my_zone_group_config = zone_cfg 
            break
        end
    end

    if not g_my_zone_group_config then
        c_err("[game_zone_mng](cache_server_list_in_my_zone_group) server:%d cfg not belongs to a zone", my_server_id)
        return 
    end

    local min_zone_id = g_my_zone_group_config.MinZoneInGroup
    local max_zone_id = g_my_zone_group_config.MaxZoneInGroup
    for server_id, server_info in pairs( g_server_list ) do
        local server_zond_id = server_info.ZoneId
        if server_zond_id >= min_zone_id and server_zond_id <= max_zone_id then
            g_my_zone_group_server_list[ server_id ] = true 
        end
    end
    g_my_server_id = g_gamesvr:c_get_server_id()
    g_my_server_name = svr_cfg["Name"]
end

--------------------------------------------------------------------------
-- 
--------------------------------------------------------------------------

function get_server_name( _server_id )
    if not g_server_list then return "" end

    local server_info = g_server_list[ _server_id ]
    if not server_info then 
        return ""
    end 
    
    return server_info["Name"]
end

function get_zone_by_server_id(_server_id)
    if not g_server_list then
        c_err("[game_zone_mng](get_zone_by_server_id) g_server_list is nil")
        return -1
    end
    local svr_cfg = g_server_list[_server_id]
    if not svr_cfg then
        c_err("[game_zone_mng](get_zone_by_server_id) not find server:%d cfg", _server_id)
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
        c_err("[game_zone_mng](get_zone_by_server_id) server:%d svr_zone_id:%d not find match zone_id in zone_list", _server_id, svr_zone_id)
        return -1
    end

    return zone_id
end

function get_zone_id()
    local server_id = g_gamesvr:c_get_server_id()
    return get_zone_by_server_id( server_id )
end

function is_in_same_group( _server_id )
    return g_my_zone_group_server_list[ _server_id ]
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

    if _server_id == g_my_server_id then
        _func( ... ) 
        return
    end

    if not g_my_zone_group_server_list[ _server_id ] then
        c_err( "[game_zone_mng](remote_call_group_server) server_id:%d is not in the same group, my server id:%d", _server_id, g_my_server_id )
        return 
    end

    bfclient.remote_call_gs_by_bf( _server_id, _func_name, ... )
end


function remote_call_group_server_ex( _server_id, _func, _func_name, ... ) 
    if g_debug then
        --函数比较容易改漏，校验一下他们是同一个东西
        local func = loadstring( sformat( "return _G.%s", _func_name ) )()
        if func ~= _func then
           c_err( "[game_zone_mng](remote_call_group_server_ex) func should be the same, name:%s", _func_name )
           c_bt()
           return
        end
    end 

    if _server_id == g_my_server_id then
        _func( ... ) 
        return
    end

    if not g_my_zone_group_server_list[ _server_id ] then
        c_err( "[game_zone_mng](remote_call_group_server_ex) server_id:%d is not in the same group, my server id:%d", _server_id, g_my_server_id )
        return 
    end

    bfclient.remote_call_gs_by_bf_ex( _server_id, _func_name, ... )
end 


function remote_call_group_bf( _bf_server_id, _func, _func_name, _server_id, ... ) 
    if g_debug then
        --函数比较容易改漏，校验一下他们是同一个东西
        local func = loadstring( sformat( "return _G.%s", _func_name ) )()
        if func ~= _func then
            c_err( "[game_zone_mng](remote_call_group_server) func should be the same, name:%s", _func_name )
            c_bt()
            return
        end
    end

    if not g_my_zone_group_server_list[ _server_id ] then
        c_err( "[game_zone_mng](remote_call_group_server) server_id:%d is not in the same group, my server id:%d", _server_id, g_my_server_id )
        return 
    end

    bfclient.remote_call_bf( _bf_server_id, _func_name, _server_id, ... )
end

function broadcast_in_group( _func, _func_name, ... ) 
    if g_debug then
        --函数比较容易改漏，校验一下他们是同一个东西
        local func = loadstring( sformat( "return _G.%s", _func_name ) )()
        if func ~= _func then
            c_err( "[game_zone_mng](remote_call_group_server) func should be the same, name:%s", _func_name )
            c_bt()
            return
        end
    end

    _func( ... ) 

    local my_server_id = g_gamesvr:c_get_server_id()
    bfclient.remote_call_zone_by_bf( my_server_id, my_server_id, _func_name, ... )
end

function is_load_zone_info_finish()
    return g_server_list ~= nil and g_zone_list ~= nil 
end

function on_load_zone_info_finish()
    if g_is_notify then
        return 
    end

    rank_list_mng.sync_rank_list_to_bf()
    wild_mng.check_open_first_wild()
    trade_mng.sync_item_list_to_bf()
    auction_mng.sync_item_list_to_bf()
    trade_statistic_mng.sync_trade_statistic_to_bf()
    guild_daily_pvp.on_gs_load_zone_info()
    event_team_ladder.on_gs_load_zone_info()
    event_team_champion.on_gs_load_zone_info()
    egg_mng.on_load_zone_info_finish()

    g_is_notify = true
end

function get_current_server_start_date() 
    if not g_server_list then 
        return 
    end
    local server_id = g_gamesvr:c_get_server_id()
    local config = g_server_list[ server_id ] 
    if not config then 
        return 
    end
    return config.ServerStartDate
end

--------------------------------------------------------------------------
-- 
--------------------------------------------------------------------------


start()
