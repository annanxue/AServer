module( "player_login_server", package.seeall )

--------------------------------------------------
-- global var
--------------------------------------------------
local sformat, unpack, floor, ostime, loadstring = string.format, unpack, math.floor, os.time, loadstring
local sgmatch, tinsert = string.gmatch, table.insert
local split_language = utils.split_language

g_plbar = g_plbar or LAr:c_new()
g_opened_gamesvr_cfg_ar = g_opened_gamesvr_cfg_ar or LAr:c_new()
g_all_gamesvr_cfg_ar = g_all_gamesvr_cfg_ar or LAr:c_new()

g_check_gamesvr_cfg_timer_ = g_check_gamesvr_cfg_timer_ or -1
g_gamesvr_cfg_version = 0
g_all_gamesvr_cfg_send_version = 0
g_opened_gamesvr_cfg_send_version = 0

g_server_list_str = ""
g_zone_list_str = ""
g_white_list_str = ""
g_server_list = nil
g_zone_list = nil
g_white_list = {}

PASSWD_MIN_LEN = 6
PASSWD_MAX_LEN = 20

PLATFORM_IOS = 1
PLATFORM_ANDROID = 2
PLATFORM_EDITOR = 3

SERVER_STATUS_MAINTAIN = 4
SERVER_STATUS_RECOMMEND = 5

SERVER_BUSY_PLAYER_NUM = 50 
SERVER_HOT_PLAYER_NUM = 200

--------------------------------------------------
-- logic function
--------------------------------------------------
local function is_in_white_list( _dpid )
    local player_ip = g_player_login_svr:c_get_player_ip( _dpid )
    local ip_tbl = {}
    for each in sgmatch( player_ip, "[^.]+" ) do
        tinsert( ip_tbl, each )
    end
    local ip_section = sformat("%d.%d.%d.*", ip_tbl[1], ip_tbl[2], ip_tbl[3])
    return g_white_list[player_ip] or g_white_list[ip_section]
end

local function is_server_in_maintain( _server_id )
    local server_config = g_server_list[_server_id]
    if server_config then
        local status = server_config.Status
        if status and status == SERVER_STATUS_MAINTAIN then
            return true
        end
    end
    return not game_login_server.g_gameserver_status[_server_id]
end

local function is_active_valid( _account_id, _server_id, _active_code )
    if _active_code == "" then
        local is_ok, active_code = db_service.get_active_info_by_account_id( _account_id, _server_id )
        if not is_ok then
            return login_code.LOGIN_NEED_ACTIVE_CODE
        else
            return login_code.LOGIN_CERTIFY_OK
        end
    else
        local is_ok, server_id, account_id = db_service.get_active_info_by_code( _active_code )
        if not is_ok then
            return login_code.LOGIN_ACTIVE_CODE_INVALID
        else
            if server_id ~= _server_id then
                return login_code.LOGIN_ACTIVE_CODE_INVALID
            end 

            if account_id == 0 then
                local update_ret = db_service.update_active_info( _account_id, _active_code ) 
                if update_ret then
                    return login_code.LOGIN_CERTIFY_OK
                else
                    return login_code.LOGIN_ACTIVE_CODE_INVALID
                end
            end

            if account_id ~= _account_id then
                return login_code.LOGIN_ACTIVE_CODE_INVALID
            else
                return login_code.LOGIN_CERTIFY_OK
            end
        end
    end
end

function verify_account_when_enter_game( _account_id, _login_token )
    local is_ok, account_info = db_service.get_account_info( _account_id )
    if not is_ok then
        return login_code.LOGIN_ACCOUNT_NOT_FOUND
    end

    --local machine_forbid_end_time = db_service.get_machine_forbid_end_time( account_info.machine_code_ )
    --if machine_forbid_end_time > ostime() then
        --c_err( "[player_login_server](verify_account_when_enter_game)machine_code: %s is forbidden, account_id: %s!", account_info.machine_code_, _account_id )
        --return login_code.LOGIN_MACHINE_FORBID, machine_forbid_end_time
    --end

    if account_info.forbid_login_ > ostime() then
        c_err( "[player_login_server](verify_account_when_enter_game)account_id: %s is forbidden!", _account_id )
        return login_code.LOGIN_ACCOUNT_FORBID, account_info.forbid_login_
    end

    if _login_token ~= account_info.login_token_ then
        c_err( "[player_login_server](verify_account_when_enter_game)account_id: %s, login token %s and %s not match", _account_id, _login_token, account_info.login_token_ )
        return login_code.LOGIN_ACCOUNT_TIMEOUT
    end
    local update_ret = db_service.invalid_account_token( _account_id )
    if update_ret < 0 then
        c_err( "[player_login_server](verify_account_when_enter_game)account_id: %s invalid token failed", _account_id )
        return login_code.LOGIN_ACCOUNT_NOT_FOUND
    end
    return login_code.LOGIN_CERTIFY_OK, account_info.queue_free_
end

function verify_and_save_account_when_enter_login( _dpid, _platform, _version, _account_id, _mcode, _sdk_time_stamp, _sdk_sign, _channel_account, _channel_id, _channel_label, _server_id, _device_name, _system_name, _active_code )
    if _platform < PLATFORM_IOS or _platform > PLATFORM_EDITOR then
        c_err( "[player_login_server](verify_and_save_account_when_enter_login)account_id: %s, platform: %d out range", _account_id, _platform )
        return false, login_code.LOGIN_VERSION_NOT_MATCH 
    end

    local server_config = g_server_list[_server_id]
    if not server_config then
        c_err( "[player_login_server](verify_and_save_account_when_enter_login)gameserver not found, account_id: %s, server_id: %d", _account_id, _server_id )
        return false, login_code.LOGIN_GAMESERVER_MAINTAINING
    end

    local in_white_list = is_in_white_list( _dpid )
    if is_server_in_maintain( _server_id ) and not in_white_list then
        c_log( "[player_login_server](verify_and_save_account_when_enter_login)gameserver is maintaining, account_id: %s, server_id: %d", _account_id, _server_id )
        return false, login_code.LOGIN_GAMESERVER_MAINTAINING
    end

    if not server_config.IsOpen and not in_white_list then
        c_err( "[player_login_server](verify_and_save_account_when_enter_login)gameserver is not open, account_id: %s, server_id: %d", _account_id, _server_id )
        return false, login_code.LOGIN_GAMESERVER_MAINTAINING
    end

    if server_config.IsNeedActive then
        local active_code_valid = is_active_valid( _account_id, _server_id, _active_code )
        if active_code_valid ~= login_code.LOGIN_CERTIFY_OK then
            return false, active_code_valid
        end
    end

    --if server_config.FirstOpenTime then
        --if server_config.FirstOpenTime > ostime() then
            --c_err( "[player_login_server](verify_and_save_account_when_enter_login)gameserver is not open unitl FirstOpenTime, account_id: %s, server_id: %d, first_open_time: %d, ostime: %d", _account_id, _server_id, server_config.FirstOpenTime, ostime())
            --return false, login_code.LOGIN_GAMESERVER_MAINTAINING
        --end
    --end

    local server_version = 0

    if _platform == PLATFORM_IOS then
        server_version = g_player_login_svr:c_get_ios_version()
    elseif _platform == PLATFORM_ANDROID then
        server_version = g_player_login_svr:c_get_android_version()
    end

    if server_version > 0 then
        if server_version <= _version then
            c_log( "[player_login_server](verify_and_save_account_when_enter_login)account_id: %s, platform: %d, client version: %d, server version: %d",
                _account_id, _platform, _version, server_version )
        else
            c_err( "[player_login_server](verify_and_save_account_when_enter_login)account_id: %s, platform: %d, client version: %d < server version: %d ", 
                _account_id, _platform, _version, server_version )
            return false, login_code.LOGIN_VERSION_NOT_MATCH 
        end
    end

    local is_ok, login_token = splus_verify.do_verify( _account_id, _sdk_time_stamp, _sdk_sign )
    if not is_ok then
        return false, login_code.LOGIN_ACCOUNT_VERIFY_FAILED
    end
    local is_new = db_service.is_new_account( _account_id )
    local update_ret = db_service.update_account_info( 
            _account_id, 
            _mcode, 
            login_token, 
            _channel_account, 
            _channel_id, 
            _channel_label, 
            _device_name, 
            _system_name )
    if update_ret < 0 then
        return false, login_code.LOGIN_ACCOUNT_VERIFY_FAILED
    end

    return true, login_code.LOGIN_CERTIFY_OK, login_token, is_new
end

function check_gamesvr_cfg()
    local base_url = g_player_login_svr:c_get_gamesvr_config_url()
    local server_list_url = sformat( "%s%s", base_url, "server_list.ini" )
    local zone_list_url = sformat( "%s%s", base_url, "zone_list.ini" )
    local white_list_url = sformat( "%s%s", base_url, "white_list.ini" )
    http_mng.request( server_list_url, "", on_server_list_response )
    http_mng.request( zone_list_url, "", on_zone_list_response )
    http_mng.request( white_list_url, "", on_white_list_response )
end

function on_server_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[player_login_server](on_server_list_response) check server list failed, response: %s", _response )
        return
    end
    if _response == g_server_list_str then
        c_log( "[player_login_server](on_server_list_response) server list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[player_login_server](on_server_list_response) update server list failed, response: %s", _response )
        return
    end
    g_server_list_str = _response
    g_server_list = func()
    g_gamesvr_cfg_version = ostime()
    -- ??server_id
    local real_server_list  = {}
    for server_id, server_config in pairs( g_server_list ) do
        local real_server_id = utils.split_number(server_id , 8)
        real_server_list[real_server_id] = server_config
    end 
    g_server_list = real_server_list


    c_log( "[player_login_server](on_server_list_response) update server list succ, version: %d", g_gamesvr_cfg_version )
end

function on_zone_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[player_login_server](on_zone_list_response) check zone list failed, response: %s", _response )
        return
    end
    if _response == g_zone_list_str then
        c_log( "[player_login_server](on_zone_list_response) zone list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[player_login_server](on_zone_list_response) update zone list failed, response: %s", _response )
        return
    end
    g_zone_list_str = _response
    g_zone_list = func()
    g_gamesvr_cfg_version = ostime()
    c_log( "[player_login_server](on_zone_list_response) update zone list succ, version: %d", g_gamesvr_cfg_version )
end

function on_white_list_response( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[player_login_server](on_white_list_response) check white list failed, response: %s", _response )
        return
    end
    if _response == g_white_list_str then
        c_log( "[player_login_server](on_white_list_response) white list not change, version: %d", g_gamesvr_cfg_version )
        return
    end

    local func = loadstring( _response )
    if not func then
        c_err( "[player_login_server](on_white_list_response) update white list failed, response: %s", _response )
        return
    end
    g_white_list_str = _response
    g_white_list = func()
    g_gamesvr_cfg_version = ostime()
    c_log( "[player_login_server](on_white_list_response) update white list succ, version: %d", g_gamesvr_cfg_version )
end

--------------------------------------------------
-- msg back
--------------------------------------------------

local function send_login_result( _dpid, _account_id, _login_token, _result_code )
    local ar = g_plbar
    ar:flush_before_send( msg.LC_TYPE_LOGIN_RESULT )
    ar:write_ushort( _result_code )
    ar:write_double( _account_id )
    ar:write_int( _login_token )
    ar:send_one_ar( g_player_login_svr, _dpid )
end

local function send_server_list( _dpid )
    local ar, send_version
    local in_white_list = is_in_white_list( _dpid )
    if in_white_list then
        ar = g_all_gamesvr_cfg_ar
        send_version = g_all_gamesvr_cfg_send_version
    else
        ar = g_opened_gamesvr_cfg_ar
        send_version = g_opened_gamesvr_cfg_send_version
    end

    if send_version ~= g_gamesvr_cfg_version then
        ar:flush_before_send( msg.LC_TYPE_SERVER_LIST )

        local server_num_offset = ar:get_offset()
        local server_num = 0
        ar:write_int( server_num )
        local date_pattern = "(%d+)/(%d+)/(%d+)"
        for server_id, server_config in pairs( g_server_list )  do
            server_num = server_num + 1
            local start_day = server_config["ServerStartDate"]
            local year, month, day = start_day:match( date_pattern )
            local startdate_timestamp = ostime({
                year = year,
                month = month,
                day = day,
                hour = 0,
                min = 0,
                sec = 0,})

            ar:write_int( server_id )
            ar:write_string( server_config["Name"] )
            ar:write_ulong( startdate_timestamp )
            ar:write_string( server_config["Ip"] )
            ar:write_int( server_config["Port"] )
            ar:write_ushort( server_config["ZoneId"] )
            ar:write_byte( server_config["Status"] or 0 )
            if server_config["IsOpen"] or in_white_list then
                ar:write_ubyte( 1 )
            else
                ar:write_ubyte( 0 )
            end
        end
        if server_num > 0 then
            ar:write_int_at( server_num, server_num_offset )
        end

        local group_num_offset = ar:get_offset()
        local group_num = 0
        ar:write_int( group_num )
        for zone_group_id, zone_group_config in pairs( g_zone_list )  do
            group_num = group_num + 1
            ar:write_ushort( zone_group_id )
            ar:write_string( zone_group_config["ZoneGroupName"] )
            ar:write_ushort( zone_group_config["MinZoneInGroup"] )
            ar:write_ushort( zone_group_config["MaxZoneInGroup"] )
        end
        if group_num > 0 then
            ar:write_int_at( group_num, group_num_offset )
        end
    end
    ar:send_one_ar( g_player_login_svr, _dpid)
    if in_white_list then
        g_all_gamesvr_cfg_send_version = g_gamesvr_cfg_version
    else
        g_opened_gamesvr_cfg_send_version = g_gamesvr_cfg_version
    end
end

local function send_server_player_num( _dpid )
    local ar = g_plbar
    ar:flush_before_send( msg.LC_TYPE_SERVER_PLAYER_NUM )

    ar:write_int( SERVER_BUSY_PLAYER_NUM )
    ar:write_int( SERVER_HOT_PLAYER_NUM )
    local server_num_offset = ar:get_offset()
    local server_num = 0
    ar:write_int( server_num )
    local gameserver_status = game_login_server.g_gameserver_status
    for server_id, server_config in pairs( g_server_list )  do
        server_num = server_num + 1
        local player_num = gameserver_status[server_id] or -1
        ar:write_int( server_id )
        ar:write_int( player_num )
    end
    ar:write_int_at( server_num, server_num_offset )
    ar:send_one_ar( g_player_login_svr, _dpid)
end

--------------------------------------------------
-- handle msg
--------------------------------------------------

function add_all_push_service( _dpid )
    local ar = g_plbar
    ar:flush_before_send( msg.LC_TYPE_PUSH_SERVICE )
    ar:write_byte( #PUSH_MESSAGES )
    for _, info in ipairs( PUSH_MESSAGES ) do
        ar:write_byte( info.PushType )
        ar:write_string( split_language(info.Message) )
        ar:write_byte( info.Month )

        local day_list = info.Day
        ar:write_ubyte( #day_list )
        for i, day in ipairs( day_list ) do
            ar:write_ubyte( day )
        end

        ar:write_byte( info.Hour )
        ar:write_byte( info.Minute )
    end
    ar:send_one_ar( g_player_login_svr, _dpid )
end

function on_player_connect( _ar, _dpid, _size )
    if g_gamesvr_cfg_version == 0 then
        c_post_disconnect_msg( g_player_login_svr, _dpid )
    else
        add_all_push_service( _dpid )
    end
end

function on_pl_get_server_info( _ar, _dpid, _size )
    if g_gamesvr_cfg_version == 0 then
        c_trace( "[player_login_server](on_pl_get_server_info) get server info before g_server_list is ok, dpid: 0x%X", _dpid )
        c_post_disconnect_msg( g_player_login_svr, _dpid )
        return
    end
    send_server_list( _dpid ) 
    send_server_player_num( _dpid )
end

function on_pl_login_call( _ar, _dpid, _size )
    local account_id = _ar:read_double()
    local mcode = _ar:read_string()
    local channel_account = _ar:read_string()
    local channel_id = _ar:read_string()
    local channel_label = _ar:read_string()
    local sdk_time_stamp = _ar:read_string()
    local sdk_sign = _ar:read_string()
    local platform = _ar:read_byte()
    local version = _ar:read_int()
    local server_id = _ar:read_int()
    local p_uid = _ar:read_string()
    local devicename = _ar:read_string()
    local systemname = _ar:read_string()
    local platform_tag = _ar:read_string()
    local active_code = _ar:read_string()

    --TODO 记录一下platform
    local is_ok, ret_code, login_token, is_new = verify_and_save_account_when_enter_login( 
        _dpid, 
        platform, 
        version, 
        account_id, 
        mcode, 
        sdk_time_stamp, 
        sdk_sign, 
        channel_account, 
        channel_id, 
        channel_label, 
        server_id,
        devicename, 
        systemname,
        active_code)

    if is_ok then
        send_login_result( _dpid, account_id, login_token, ret_code )
        c_login( "[player_login_server](on_pl_login_call)account_id: %s, channel_id: %s, channel_label: %s, version: %d, certify success", 
            account_id, channel_id, channel_label, version )
        if is_new then
            logclient.log_account( account_id, channel_account, _dpid, mcode, channel_id, channel_label, p_uid, devicename, systemname, p_uid, platform_tag )
        end
    else
        send_login_result( _dpid, account_id, 0, ret_code )
        c_login( "[player_login_server](on_pl_login_call)account_id: %s, channel_id: %s, channel_label: %s, version: %d, certify failed", 
            account_id, channel_id, channel_label, version )
    end

end

function on_pl_machine_info( _ar, _dpid, _size )
    local account_id = _ar:read_uint()
    local cpu_name = _ar:read_string()
    local cpu_count = _ar:read_byte()
    local shader_level = _ar:read_byte()
    local render_target_count = _ar:read_byte()
    local is_support_shadow = _ar:read_byte()
    local gpu_name = _ar:read_string()
    local gpu_memory = _ar:read_int()

    c_login( sformat( 
        [[
        MACHINE INFO:
        ACCOUNT ID:%d 
            CPU NAME: %s
            CPU COUNT: %d
            GPU NAME: %s
            GPU MEMORY: %dM
            SHADER_LEVEL: %d.%d
            RENDER_TARGET_NUM: %d
            SUPPORT SHADOW: %d
            ]],
        account_id,
        cpu_name,
        cpu_count,
        gpu_name,
        gpu_memory,
        floor( shader_level/10 ), shader_level%10,
        render_target_count,
        is_support_shadow
        )
    )
end

function send_ping( _dpid, _time )
    local ar = g_plbar
    ar:flush_before_send( msg.ST_PING )
    ar:write_uint( _time )
    ar:send_one_ar( g_player_login_svr, _dpid )
end

function on_ping( _ar, _dpid, _size )
    local ping_time = _ar:read_uint()
    local unity_time = _ar:read_uint()
    send_ping( _dpid, ping_time )
end

function on_client_err_log( _ar , _dpid , _size )
    local err_log = _ar:read_string()
    c_err( "[player_login_server] (on_client_err_log) dpid: 0x%X, client error log: \n%s", _dpid, err_log )
end

function on_app_pause( _ar , _dpid , _size )
    -- Do Nothing
end

func_table =
{
    [msg.PT_CONNECT]                           = on_player_connect,
    [msg.CL_TYPE_LOGIN]                        = on_pl_login_call,
    [msg.CL_TYPE_MACHINE_INFO]                 = on_pl_machine_info,
    [msg.CL_TYPE_GET_SERVER_INFO]              = on_pl_get_server_info,
    [msg.PT_PING]                              = on_ping,
    [msg.PT_CLIENT_ERR_LOG]                    = on_client_err_log,
    [msg.PT_MINISERVER_ERR_LOG]                = on_client_err_log,
    [msg.PT_APP_PAUSE]                         = on_app_pause,
}

--------------------------------------------------
-- callback functions for c/c++
--------------------------------------------------
function message_handler( _ar, _msg, _size, _packet_type, _dpid )
    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        c_err( "[player_login_server](message_handler) unknown packet 0x%08x %08x", _packet_type, _dpid )
    end  
end

function start()
    if g_check_gamesvr_cfg_timer_ ~= -1 then
        g_timermng:c_del_timer( g_check_gamesvr_cfg_timer_ )
    end
    g_check_gamesvr_cfg_timer_ = g_timermng:c_add_timer_second( 0, timers.CHECK_GAMESVR_CFG, 0, 0, 60 )
end

start()
