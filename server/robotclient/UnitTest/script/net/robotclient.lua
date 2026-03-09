module( "robotclient", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber
local mfloor, mrandom, mmin, ostime, osdifftime = math.floor, math.random, math.min, os.time, os.difftime
local tconcat = table.concat

local ROBOT_TYPE_DO_NOTHING      = 1
local ROBOT_TYPE_RANDOM_MOVE     = 2
local ROBOT_TYPE_RANDOM_SKILL    = 3
local ROBOT_TYPE_SHOP            = 4
local ROBOT_TYPE_CHAT            = 5
local ROBOT_TYPE_RECHARGE        = 6
local ROBOT_TYPE_INSTANCE        = 7
local ROBOT_TYPE_TEAM            = 8
local ROBOT_TYPE_MATCH           = 9
local ROBOT_TYPE_TRADE           = 10

DPID_TYPE_LOGIN = 1
DPID_TYPE_GAME = 2
DPID_TYPE_BF = 3

g_ar = g_ar or LAr:c_new()

g_dpid_mng = g_dpid_mng or {}  -- 每个dpid对应一个常规客户端

g_pending_certify = g_pending_certify or {}  -- 等待连接gameserver并发送PT_CERTIFY的帐号
g_migrating_heros = g_migrating_heros or {}  -- 等待连接bfserver并跨服的主角
g_rejoining_heros = g_rejoining_heros or {}  -- 等待从bf返回game的主角

g_login_queue_test = g_login_queue_test or false
g_quit_queue_timer_map = g_quit_queue_timer_map or {}

g_robot_type = 0

g_user_id_count = g_user_id_count or 0 

function on_request_sdk_server_login_info( _user_params, _response, _post_data, _is_request_succ )
    if not _is_request_succ then
        c_err( "[ROBOT](on_request_sdk_server_login_info)post_data: %s, http request failed", _post_data )
        return
    end

    local login_dpid = _user_params
    local login_info = cjson.decode( _response )

    if login_info.data and login_info.code then
        if login_info.code ~= 1 then
            c_err( "[ROBOT](on_request_sdk_server_login_info)post_data: %s, response code: %d", login_info.code )
            return
        end

        login_info = login_info.data
    end

    local sdk_uid = tonumber( login_info["uid"] )
    local sdk_sign = login_info["sign"]
    local sdk_time_stamp = login_info["timestamp"]
    local sdk_channel_account = login_info["p_uid"]
    local sdk_channel_id = login_info["channelId"]
    local sdk_channel_label = login_info["channelLable"]

    local mcode = "robotclient_"..c_rand_str(8)   
    local account_name = sdk_uid
    local gamesvr_id = g_robotclient:c_get_gamesvr_id()

    local account = {}
    account.mcode_ = mcode
    account.account_name_ = account_name
    account.account_id_ = sdk_uid
    g_dpid_mng[login_dpid] = account

    local ar = g_ar
    ar:flush_before_send( msg.CL_TYPE_LOGIN )
    ar:write_double( sdk_uid )
    ar:write_string( mcode )
    ar:write_string( sdk_channel_account )
    ar:write_string( sdk_channel_id )
    ar:write_string( sdk_channel_label )
    ar:write_string( sdk_time_stamp )
    ar:write_string( sdk_sign )
    ar:write_byte( 3 )  -- platform: EDITOR
    ar:write_int( 0 )  -- version
    ar:write_int( gamesvr_id )
    ar:write_string("not_robot_puid")
    ar:write_string("not_robot_devicename")
    ar:write_string("not_robot_systemname")
    ar:write_string("google")
    ar:write_string("")
    ar:send_one_ar( g_robotclient, login_dpid )

    c_log( sformat( "[ROBOT](on_request_sdk_server_login_info) account_id: %d, try login to loginserver", sdk_uid ) )
end

function on_request_sdk_server_login_info2( _user_params, _response, _post_data, _is_request_succ )
end

function set_user_id_begin_num( _begin_num )
    g_user_id_count = _begin_num
    if g_user_id_count == 0 then
        local math = math
        math.randomseed( os.time() )
        g_user_id_count = math.random( 500000 ) * 2000
    end
    c_log( sformat( "[ROBOT](set_user_id_begin_num)robot user id begin num: %d", g_user_id_count ) )
end

function set_robot_type( _type )
    g_robot_type = _type
    c_log( "[ROBOT](set_robot_type) robot type: %d", g_robot_type )
end

function auto_create_account( _login_dpid )
    g_user_id_count = g_user_id_count + 1 

    local login_type = g_robotclient:c_get_login_type()

    if login_type == "" or login_type == "test" then
        local post_data = "partner_account=" .. g_user_id_count 

        local is_task_send = http_mng.request( http_mng.TEST_LOGIN_URL, post_data, on_request_sdk_server_login_info, _login_dpid )

        if is_task_send then
            c_log( sformat( "[ROBOT](auto_create_account)request login info from sdk server. post_data: %s", post_data ) )
        else
            c_err( "[ROBOT](auto_create_account)user_id: %d, post_data: %s, http request failed", g_user_id_count, post_data )
        end
    elseif login_type == "39" then
        --[[
        local data = { name = "test", accesstoken = "test", uid = "" .. g_user_id_count }
        local json_data = cjson.encode( data )
        local sign_key = ")%@*(#%IJDGkkdgn12"
        local platform = "pt39"
        local timestamp = os.time()
        local app_id = "1000002"
        local login_type = "pt39"
        local sign = c_md5( json_data .. sign_key .. platform .. timestamp .. "1fead46e83950761d919b24481171cd4" )

        local post_data= sformat( "data=%s&signkey=%s&platform=%s&timestamp=%s&appId=%s&loginType=%s&sign=%s", 
            json_data, sign_key, platform, timestamp, app_id, login_type, sign )

            --for i = 1, 20, 1 do
                local is_task_send = http_mng.request( "https://zuidaguai.com/LoginDemo", post_data, on_request_sdk_server_login_info, _login_dpid )
                if is_task_send then
                    c_log( "[ROBOT](auto_create_account)request login info from sdk server. post_data: %s", post_data )
                else
                    c_err( "[ROBOT](auto_create_account)user_id: %d, post_data: %s, http request failed", g_user_id_count, post_data )
                end
            --end

            for i = 1, 20, 1 do
                local is_task_send = http_mng.request( "https://zuidaguai.com/LoginDemo", post_data, on_request_sdk_server_login_info2, _login_dpid )
            end
        ]]
    end
end

function on_loginsvr_connect( _dpid )
    local dpid_info = {}
    dpid_info.dpid_type_ = DPID_TYPE_LOGIN
    g_dpid_mng[_dpid] = dpid_info

    c_log( sformat( "[ROBOT](on_loginsvr_connect) dpid: 0x%X", _dpid ) )

    auto_create_account( _dpid )
end

function on_gamesvr_connect( _dpid )
    local dpid_info

    c_log( sformat( "[ROBOT](on_gamesvr_connect) dpid: 0x%X", _dpid ) )
    g_timermng:c_add_timer_second( 6, timers.PING, _dpid, 0, 6 )

    if #g_pending_certify ~= 0 then
        dpid_info = tremove( g_pending_certify )
        send_certify_to_game( _dpid, dpid_info )
    elseif #g_rejoining_heros ~= 0 then
        dpid_info = tremove( g_rejoining_heros )
        send_rejoin( _dpid, dpid_info )
        dpid_info.rejoin_certify_code_ = nil
    else
        -- TODO: 断线重连后自动进游戏
    end

    if dpid_info then
        dpid_info.dpid_type_ = DPID_TYPE_GAME
        g_dpid_mng[_dpid] = dpid_info
        ctrl_mng.add_dpid( _dpid )
    end
end

function on_bfsvr_connect( _dpid, _ip, _port )
    c_log( sformat( "[ROBOT](on_bfsvr_connect) dpid: 0x%X, ip: %s, port: %d", _dpid, _ip, _port ) )
    local bf_addr = sformat( "%s:%d", _ip, _port )
    local dpid_info = tremove( g_migrating_heros[bf_addr] )
    if dpid_info then  -- 如果bf的连接意外断开又重连上的话，dpid_info可能为空
        dpid_info.dpid_type_ = DPID_TYPE_BF
        local player_id = dpid_info.player_id_
        local migrate_info = dpid_info.migrate_info_
        local ar = g_ar
        ar:flush_before_send( msg.PT_ASK_BF_REGISTER_DPID )
        ar:write_int( migrate_info.gamesvr_id_ )
        ar:write_ulong( player_id )
        ar:write_int( migrate_info.bf_certify_code_ )
        ar:send_one_ar( g_robotclient, _dpid )
        dpid_info.migrate_info_ = nil
        g_dpid_mng[_dpid] = dpid_info
        ctrl_mng.add_dpid( _dpid )
    end
end

local function add_migrating_hero( _dpid_info )
    local migrate_info = _dpid_info.migrate_info_
    local bf_ip = migrate_info.bf_ip_
    local bf_port = migrate_info.bf_port_
    local bf_addr = sformat( "%s:%d", bf_ip, bf_port )
    local pending_migrate_of_one_bf = g_migrating_heros[bf_addr]
    if not pending_migrate_of_one_bf then
        pending_migrate_of_one_bf = {}
        g_migrating_heros[bf_addr] = pending_migrate_of_one_bf
    end
    tinsert( pending_migrate_of_one_bf, _dpid_info )
    g_robotclient:c_connect_bfsvr( bf_ip, bf_port )
end

local function add_rejoining_hero( _dpid_info )
    tinsert( g_rejoining_heros, _dpid_info )
    g_robotclient:c_connect_gamesvr()
end

function on_disconnect( _ar, _dpid, _size )
    local dpid_info = g_dpid_mng[_dpid]
    if dpid_info then
        dpid_info.skill_secret_ = nil
        local dpid_type = dpid_info.dpid_type_

        if dpid_type == DPID_TYPE_GAME or dpid_type == DPID_TYPE_BF then
            dpid_info.hero_ctrl_id_ = nil
            ctrl_mng.remove_dpid( _dpid )

            if dpid_type == DPID_TYPE_GAME then
                if dpid_info.migrate_info_ then  -- 即将迁移到bf
                    add_migrating_hero( dpid_info )
                end
            elseif dpid_type == DPID_TYPE_BF then
                if dpid_info.rejoin_certify_code_ then  -- 即将从bf返回game
                    add_rejoining_hero( dpid_info )
                end
            end
        end

        g_dpid_mng[_dpid] = nil
    end
end

function on_create_account_result( _ar, _dpid, _size )
    local err_no = _ar:read_ushort()
    local account_id = _ar:read_long()
    local account_name = _ar:read_string()
    local encrypted_passwd = _ar:read_string()

    local dpid_info = g_dpid_mng[_dpid]

    if err_no == login_code.CREATE_ACCOUNT_OK then
        c_log( sformat( "[ROBOT](on_create_account_result) created account '%s', account id: %d.", account_name, account_id ) )
        dpid_info.account_id_ = account_id
        dpid_info.encrypted_passwd_ = encrypted_passwd
        g_robotclient:c_disconnect( _dpid )
        tinsert( g_pending_certify, dpid_info )
        g_robotclient:c_connect_gamesvr()
    elseif err_no == login_code.CREATE_ACCOUNT_EXIST then
        c_err( sformat( "[ROBOT](on_create_account_result) cannot create account '%s', account already exists.", dpid_info.account_name_ ) )
        auto_create_account( _dpid )  -- retry
    elseif err_no == login_code.CREATE_ACCOUNT_DB_FAILED then
        c_err( sformat( "[ROBOT](on_create_account_result) cannot create account '%s', db error.", dpid_info.account_name_ ) )
    else
        c_err( sformat( "[ROBOT](on_create_account_result) cannot create account '%s', unkown error.", dpid_info.account_name_ ) )
    end
end

function on_game_certify_result( _ar, _dpid, _size )
    local retcode = _ar:read_ushort()
    local certify_key = _ar:read_int()
    if retcode == login_code.CERTIFY_OK then
        c_log( "[ROBOT](on_game_certify_result) certify ok" )
    else
        c_err( "[ROBOT](on_game_certify_result) certify error, error code: %d", retcode )
    end
end

function on_game_player_list( _ar, _dpid, _size )
    local dpid_info = g_dpid_mng[_dpid]
    
    local player_cnt = _ar:read_ubyte()
    local player_list = {}
    for i = 1, player_cnt, 1 do
        local player = 
        {
            player_id_ = _ar:read_ulong(),
            model_id_ = _ar:read_ulong(),
            use_model_ = _ar:read_ubyte(),
            model_level_ = _ar:read_ubyte(),
            job_id_ = _ar:read_ulong(),
            player_name_ = _ar:read_string(),
            level_ = _ar:read_ulong(),
            slot_ = _ar:read_ulong(),
            total_gs_ = _ar:read_ulong(),
            wing_id_ = _ar:read_ulong(),
            active_weapon_sfxs_ = _ar:read_string(),
            need_download = _ar:read_byte(),
        }
        tinsert( player_list, player )
    end
    local rand_key_join = _ar:read_ulong()
    local last_join_player_id = _ar:read_ulong()

    c_log( "[ROBOT](on_game_player_list) dpid: 0x%X, account: %s, number of players: %d", _dpid, dpid_info.account_name_, player_cnt )

    dpid_info.player_list_ = player_list
    dpid_info.rand_key_join_ = rand_key_join

    if player_cnt == 0 then
        send_get_random_name( _dpid )
    else
        local player = player_list[1]
        send_join( _dpid, player.player_id_, dpid_info.rand_key_join_ )
    end
end

function on_random_name( _ar, _dpid, _size )
    local dpid_info = g_dpid_mng[_dpid]
    local player_name = _ar:read_string()
    local time_now = ostime()
    local player_name_my = "robot_"..time_now
    c_log("random_name:%s;my_name:%s", player_name, player_name_my);
    send_auto_create_player( _dpid, dpid_info, player_name_my )
end

function on_create_player_result( _ar, _dpid, _size )
    local retcode = _ar:read_int()
    if retcode ~= login_code.CREATE_PLAYER_OK then
        c_err( "[robotclient](on_game_create_player_result) dpid: 0x%X, retcode: %d", _dpid, retcode )
    end
end

function on_game_delete_player_result( _ar, _dpid, _size )
	local retcode = _ar:read_ubyte()
    local msg
    if retcode == login_code.DELETE_PLAYER_OK then
        msg = "ok"
    elseif retcode == login_code.DELETE_PLAYER_ACCOUNT_NOT_FOUND  then
        msg = "account not found"
    elseif retcode == login_code.DELETE_PLAYER_PLAYER_NOT_FOUND  then
        msg = "player not found"
    elseif retcode == login_code.DELETE_PLAYER_SQL_ERR then
        msg = "sql error"
    else
        msg = "unknown return code"
    end
    c_log( sformat( "[ROBOT](on_game_delete_player_result) dpid: %d; message: %s", _dpid, msg ) )
end

function on_game_join_player_result( _ar, _dpid, _size )
    local retcode = _ar:read_ubyte()
    c_trace( sformat( "[ROBOT](on_game_join_player_result)player join code: %d", retcode ) )
end

function on_game_login_other_place( _ar, _dpid, _size )
    c_log( "[ROBOT](on_game_login_other_place) your account is logging in from other place." )
end

function on_game_already_login_other( _ar, _dpid, _size )
    c_log( "[ROBOT](on_game_already_login_other) your account has already logged in from other place." )
end

function on_login_queue_full( _ar, _dpid, _size )
    c_log( "[ROBOT](on_login_queue_full) dpid: 0x%X server login queue is full, you'd better choose another game server", _dpid )
end

function on_login_queue_enqueue( _ar, _dpid, _size )
    local rank = _ar:read_int()
    if g_login_queue_test then
        g_quit_queue_timer_map[ _dpid ] = g_timermng:c_add_timer_second( math.random( 60, 10 * 60 ), timers.AUTO_QUIT_QUEUE_TEST_TIMER, _dpid, 0, 0 )
    end
    c_log( "[ROBOT](on_login_queue_enqueue) dpid: 0x%X join login queue at the rank %d", _dpid, rank )
end

function on_login_queue_rank( _ar, _dpid, _size )
    local rank = _ar:read_int()
    c_log( "[ROBOT](on_login_queue_rank) dpid: 0x%X update login queue rank: %d", _dpid, rank )
end

function on_login_queue_quit_ret( _ar, _dpid, _size )
    local ret = _ar:read_ubyte()
    if ret > 0 then
        send_disconnect( _dpid )
    end
    c_log( "[ROBOT](on_login_queue_quit_ret) dpid: 0x%X quit ret: %d", _dpid, ret )
end

function get_hero( _dpid )
    local dpid_info = g_dpid_mng[_dpid]
    if not dpid_info then
        return nil
    end
    local hero_ctrl_id = dpid_info.hero_ctrl_id_
    if not hero_ctrl_id then
        return nil
    end
    return ctrl_mng.get_ctrl( _dpid, hero_ctrl_id )
end

local function after_hero_added( _dpid, _hero )
    local dpid_info = g_dpid_mng[_dpid]
    local ctrl_id = _hero.ctrl_id_

    dpid_info.hero_ctrl_id_ = ctrl_id
    _hero.dpid_ = _dpid

    send_hero_join_succ( _dpid )

    c_log( "[ROBOT](after_hero_added) ctrl_id: %d, dpid: 0x%X", ctrl_id, _dpid )

    if robot_type_func_table[g_robot_type] then 
        (robot_type_func_table[g_robot_type])( _dpid, _hero )
    else 
        c_err( "[ROBOT](after_hero_added) unknown robot type: %d", g_robot_type )
    end 

    if g_login_queue_test then
        if g_quit_queue_timer_map[ _dpid ] then
            g_timermng:c_del_timer( g_quit_queue_timer_map[ _dpid ] ) 
            g_quit_queue_timer_map[ _dpid ] = nil 
        end
        g_timermng:c_add_timer_second( mrandom( 5 * 60, 10 * 60 ) , timers.AUTO_DISCONNECT_TEST_TIMER, _dpid, 0, 0 ) 
    end
end

function on_add_obj( _ar, _dpid, _size )
    local is_active = _ar:read_byte()
    local obj = obj_t.unserialize( _ar, is_active )

    local dpid_info = g_dpid_mng[_dpid]

    local ctrl_id = obj.ctrl_id_
    --ctrl_mng.add_ctrl( _dpid, ctrl_id, obj ) -- 只加主角， 其他用不上，徒增内存和cpu update

    -- if obj:is_player() then
    --     c_log( "[ROBOT](on_add_obj) player added, dpid: 0x%X, ctrl_id: %d, player_id: %d, player_name: %s", _dpid, ctrl_id, obj.player_id_, obj.player_name_ )
    -- elseif obj:is_monster() then
    --     c_log( "[ROBOT](on_add_obj) monster added, dpid: 0x%X, ctrl_id: %d, type_id: %d", _dpid, ctrl_id, obj.type_id_ )
    -- elseif obj:is_trigger() then
    --     c_log( "[ROBOT](on_add_obj) trigger added, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    -- elseif obj:is_bullet() then
    --     c_log( "[ROBOT](on_add_obj) bullet added, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    -- else
    --     c_log( "[ROBOT](on_add_obj) obj added, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    -- end

    if is_active > 0 and obj:is_player() then  -- 主角
        ctrl_mng.add_ctrl( _dpid, ctrl_id, obj )
        obj.dpid_ = _dpid 
        after_hero_added( _dpid, obj )
    else
        obj.buffs_ = {}
        if obj:is_bullet() then obj.core_:c_delete() end
        obj:delete()
    end
end

function on_spawn_finish( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
end

function on_add_equip_info( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local count = _ar:read_byte()
    for i = 1, count do
        local part = _ar:read_byte()
        local index = _ar:read_ushort()
    end
end

function on_rm_obj( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong() 
    ctrl_mng.remove_ctrl( _dpid, ctrl_id )
    --c_log( sformat( "[ROBOT](on_rm_obj) ctrl_id: %d removed", ctrl_id ) )
end

function on_game_snapshot( _ar, _dpid, _size )
    local cb = _ar:read_short()
    local old_cb = cb
    local snapshot_his = {}
    for i = 1, cb, 1 do
        local hdr = _ar:read_byte()
        tinsert( snapshot_his, sformat("0x%X", hdr) )

        c_trace("Recv Msg:%s  Id:0x%08x",  msg_id_2_name(hdr), hdr)

        if( func_table[hdr] ) then 
            (func_table[hdr])( _ar, _dpid, _size )
        else
            c_err( sformat( "[snpashot]packet: 0x%X not found, cb count: %d, handle count: %d, hdr his:%s", hdr, old_cb, i-1, tconcat( snapshot_his, "," ) ) )
            --c_debug()
            break
        end
    end
end

function on_move_to_pos( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local x = _ar:read_float()
    local z = _ar:read_float()
end

function on_move_ground( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local degree = _ar:read_int_degree()
end

function on_set_angle( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local degree = _ar:read_int_degree()
end

function on_corr_pos( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
    local move_to_type = _ar:read_ubyte()
end

function on_state_stand( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
end

function on_trace_pos( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local x = _ar:read_float()
    local z = _ar:read_float()
    local angle = _ar:read_float()
end

function on_set_camp( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local camp = _ar:read_byte()
end

function on_add_damage( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local ttype = _ar:read_byte()
    local stype = _ar:read_byte()
    local etype = _ar:read_byte()
    local damage = _ar:read_ulong()
    local shield_damage = _ar:read_ulong()
    local skill_id = _ar:read_ulong()
    local attack_id = _ar:read_ulong()
end

function on_state_dead( _ar, _dpid, _size )
    local skill_id = _ar:read_ulong()
    local ctrl_id = _ar:read_ulong()
    local attcker_id = _ar:read_ulong()
    local ttype = _ar:read_byte()
    local angle = _ar:read_int_degree()
    if ttype == skill.DTYPE_FLY or ttype == skill.DTYPE_BACK then
        local speed = _ar:read_float()
        local tx = _ar:read_float()
        local tz = _ar:read_float()
        local hit_out_type = _ar:read_byte()
    end
    if ttype == skill.DTYPE_FLY then
        local fly_time = _ar:read_uint()
    end
    local revive_count = _ar:read_int()

    local ar = g_ar     
    ar:flush_before_send( msg.PT_REVIVAL )
    ar:send_one_ar( g_robotclient, _dpid )
end

function on_add_buff( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local caster_id = _ar:read_ulong()
    local buff_sn = _ar:read_ulong()
    local buff_id = _ar:read_ulong()
    local group_id = _ar:read_ulong()
    local time = _ar:read_ulong()
    local online_sfx = _ar:read_byte()

    -- client params
    local client_param_cnt = _ar:read_int()
    for i = 1, client_param_cnt do
        local effect_id = _ar:read_int()
        local param_cnt = _ar:read_int()
        for j = 1, param_cnt do
            local param = _ar:read_int()
        end
    end
end

function on_rm_buff( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local buff_sn = _ar:read_ulong()
end

function on_skill_jump( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local skill_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
    local tx = _ar:read_float()
    local tz = _ar:read_float()
end

function on_state_hurt( _ar, _dpid, _size )
    local skill_id = _ar:read_ulong()
    local attcker_id = _ar:read_ulong()
    local ctrl_id = _ar:read_ulong()
    local ttype = _ar:read_byte()
    local angle = _ar:read_int_degree()
    if ttype == skill.HTYPE_BACK or ttype == skill.HTYPE_DOWN or ttype == skill.HTYPE_FLYOFF then
        local speed = _ar:read_float()
        local tx = _ar:read_float()
        local tz = _ar:read_float()
    end
end

function on_state_navigate( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
    local tx = _ar:read_float()
    local tz = _ar:read_float()
    local nav_rate = _ar:read_float()
    local nav_type = _ar:read_int()
end

function on_set_value( _ar, _dpid, _size )
    local server = _ar:read_byte()
    local ctrl_id = _ar:read_ulong()
    local index = _ar:read_byte()
    local write_func, read_func = player_attr.get_attr_send_func( index )
    local value = read_func( _ar )
    local hero = get_hero( _dpid )
    if not hero then return end
    if index == JOBS.JOB_ATTR_INDEX.gold_ then
        hero.gold_ = value
    end
end

function on_state_skill( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local skill_id = _ar:read_uint()
    local x, z, angle = _ar:read_compress_pos_angle()
    local tx = _ar:read_float()
    local tz = _ar:read_float()
    local is_request = _ar:read_byte()
    local target_id = _ar:read_ulong()
end

function on_state_dazed( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local attacker_id = _ar:read_ulong()
    local seconds = _ar:read_float()
    local cfg_id = _ar:read_int()
end

function on_state_hold( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local attacker_id = _ar:read_ulong()
    local seconds = _ar:read_float()
end

function on_trigger_state( _ar, _dpid, _size )
	local trg_id = _ar:read_ulong()
	local trg_state = _ar:read_ulong()
end

function on_trigger_action( _ar, _dpid, _size )
	local trg_id = _ar:read_ulong()
	local trg_action = _ar:read_ulong()
end
	
function on_state_mst_nav( _ar)--12
    local ctrl_id = _ar:read_ulong()
    on_add_mst_nav(_ar)
end

function on_add_mst_nav( _ar )
    local ani_type = _ar:read_byte()
    local dir_angle = _ar:read_float()
    local path_len = _ar:read_byte()

    for i = 1, path_len do
        local x = _ar:read_ushort() * 0.1
        local z = _ar:read_ushort() * 0.1
    end
    local turn_type = _ar:read_byte()
    local rush_master_rate = _ar:read_float()
end

function on_add_mst_stand( _ar )
    local ctrl_id = _ar:read_ulong()
    local ani_name = _ar:read_byte()

    local pos1 = _ar:read_float()
    local pos2 = _ar:read_float()

    local act_type = _ar:read_byte()
    local turn_type = _ar:read_byte()
    local tar_id = _ar:read_ulong()
    local dir_radian = _ar:read_float()
end

function on_add_mst_path(_ar)
    local m = _ar:read_byte()
    if m == msg.ST_STATE_MST_NAVIGATE then
        on_add_mst_nav(_ar)
    elseif m == msg.ST_STATE_MST_STAND then
        on_add_mst_stand(_ar)
    end
end

function on_state_msg_stand( _ar) --12
    local ctrl_id = _ar:read_ulong()
    on_add_msg_stand(_ar)
end

function on_mst_attackable( _ar) --12
    local ctrl_id = _ar:read_ulong()
    local attackable = _ar:read_byte()
end

function on_add_msg_stand(_ar)
    local ani_index = _ar:read_byte()
    local x = _ar:read_float()
    local z = _ar:read_float()
    local act_type = _ar:read_byte()
    local turn_type = _ar:read_byte()
    local tar_id = _ar:read_ulong()
    local dir_radian = _ar:read_float()
end

function on_state_rush( _ar )
    local ctrl_id = _ar:read_ulong()
    local skill_Id = _ar:read_uint()
    local speed = _ar:read_float()
    local tx = _ar:read_float()
    local tz = _ar:read_float()
end

function on_state_jump( _ar )
    local ctrl_id = _ar:read_ulong()
    local skill_Id = _ar:read_uint()
    local speed = _ar:read_float()
    local tx = _ar:read_float()
    local tz = _ar:read_float()
end

function on_move_persist_cancel( _ar )
    local ctrl_id = _ar:read_ulong()
end

function on_state_move_persist_pause( _ar )
    local ctrl_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
end

function on_trace_state( _ar )
    local ctrl_id = _ar:read_ulong()
    local state = _ar:read_int()
end

function on_skill_failed( _ar )
    local obj_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
end

function on_add_sfx( _ar )
    local x = _ar:read_float()
    local z = _ar:read_float()
    local sfx_id = _ar:read_int()
end

function on_change_color( _ar )
    local obj_id = _ar:read_ulong()
    local material_type = _ar:read_byte()
    local color_type = _ar:read_byte()
    local time = _ar:read_float()
end

function on_change_scale( _ar )
    local obj_id = _ar:read_ulong()
    local scale = _ar:read_float()
    local time = _ar:read_float()
end

function on_revival( _ar )
    local obj_id = _ar:read_ulong()
    local x, z, angle = _ar:read_compress_pos_angle()
end

function on_unlock_job_skills( _ar, _dpid, _size )
    local skill_cnt = _ar:read_int()
    for i = 1, skill_cnt do
        local skill_id = player_t.unserialize_one_job_skill_for_client( _ar )
    end
end

function on_unlocked_talents( _ar, _dpid, _size )
    player_t.unserialize_talents_for_client( _ar )
end

function on_level_up( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local level = _ar:read_ushort()
    c_log( "[robotclient](on_level_up) ctrl_id: %d, level: %d", ctrl_id, level )
end

function on_add_item( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local item = item_t.unserialize_item_for_client( _ar )
    local hero = get_hero( _dpid )
    if not hero then return end
    tinsert( hero.bag_.item_list_, item )
    c_item_c( "[robotclient](on_add_item) dpid: 0x%X, ctrl_id: %d, item_id: %d", _dpid, ctrl_id, item.item_id_ )
end

function on_remove_item( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local index = _ar:read_int()
    local hero = get_hero( _dpid )
    if not hero then return end
    tremove( hero.bag_.item_list_, index )
    c_item_c( "[robotclient](on_remove_item) _dpid: 0x%X, ctrl_id: %d, index: %d", _dpid, ctrl_id, index )
end

function on_remove_item_complete( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
end

function on_update_item( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local index = _ar:read_int()
    local item = item_t.unserialize_item_for_client( _ar )
    local hero = get_hero( _dpid )
    if not hero then return end
    hero.bag_.item_list_[index] = item
    c_item_u( sformat( "[ROBOT](on_update_item) dpid: %d, ctrl_id: %d, index: %d, item_id: %d", _dpid, ctrl_id, index, item.item_id_ ) )
end

function on_set_job_skill( _ar, _dpid, _size )
    local skill_id = player_t.unserialize_one_job_skill_for_client( _ar )
end

function on_migrate_bf_begin( _ar, _dpid, _size )
    local gamesvr_id = _ar:read_int()
    local bf_ip = _ar:read_string()
    local bf_port = _ar:read_ushort()
    local bf_certify_code = _ar:read_int()

    c_log( sformat( "[ROBOT](on_migrate_bf_begin) dpid: %d, bf_ip: %s, bf_port: %d, certify_code: %u, server_id: %d", _dpid, bf_ip, bf_port, bf_certify_code, gamesvr_id ) )

    local migrate_info = {}
    migrate_info.bf_ip_ = bf_ip
    migrate_info.bf_port_ = bf_port
    migrate_info.bf_certify_code_ = bf_certify_code
    migrate_info.gamesvr_id_ = gamesvr_id

    local dpid_info = g_dpid_mng[_dpid]
    dpid_info.migrate_info_ = migrate_info

    local ar = g_ar
    ar:flush_before_send( msg.PT_SEND_MIGRATE_BEGIN_SUCC )
    ar:send_one_ar( g_robotclient, _dpid )

    g_robotclient:c_add_nonreconnect_dpid( _dpid )  -- wait for gamesvr to kick myself
end

function on_rejoin_game_begin( _ar, _dpid, _size )
    local dpid_info = g_dpid_mng[_dpid]
    dpid_info.rejoin_certify_code_ = _ar:read_int()
    local ar = g_ar
    ar:flush_before_send( msg.PT_SEND_REJOIN_BEGIN_SUCC )
    ar:send_one_ar( g_robotclient, _dpid )
    g_robotclient:c_add_nonreconnect_dpid( _dpid )  -- 等待bf断线
end

function on_action_sfx( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local sfx_id = _ar:read_ulong()
    local is_open = _ar:read_byte()
end

function on_corr_pos_hero( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local x, z, angle_y = _ar:read_compress_pos_angle()
    local ctrl = ctrl_mng.get_ctrl( _dpid, ctrl_id )
    local core = ctrl.core_
    core:c_setpos( x, 0, z )
    core:c_set_angle_y( angle_y )
end

function on_time_ctrl_refresh( _ar, _dpid, _size )
    local cnt = _ar:read_int()
    for i = 1, cnt do
        local id = _ar:read_int()
        local val = _ar:read_int()
    end
end

function on_ask_quest_world_map_info( _ar, _dpid, _size )
    quest_worldmap_t.unserialize_quest_worldmap_for_client( _ar )
end

function on_state_cinema( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local enter_state = _ar:read_byte()
    local cinema_type = _ar:read_byte()
    if cinema_type == 1 then  -- 副本内跳转
        local x = _ar:read_float()
        local z = _ar:read_float()
        local angle = _ar:read_float()
    end
end

function on_player_rune_rank_attr( _ar, _dpid, _size )
    local attr_cnt = _ar:read_byte()
    for i = 1, attr_cnt do
        local attr_val = _ar:read_int()
    end
end

function on_add_mail( _ar, _dpid, _size )
    local mail = mail_t.unserialize_mail_for_client( _ar )
end

function on_defined_text( _ar, _dpid, _size )
    local text_id = _ar:read_int()
    c_log( "[robotclient](on_defined_text) _dpid: 0x%X, text_id: %d", _dpid, text_id )
end

function on_defined_text_str( _ar, _dpid, _size )
    local text_id = _ar:read_int()
    local arg_num = _ar:read_int()
    for i = 1, arg_num, 1 do
        local param_type = _ar:read_byte()
        if param_type == share_const.RAW_TEXT then
            _ar:read_string()
        elseif param_type == share_const.ITEM_ID then
            _ar:read_int()
        end
        --local arg = _ar:read_string()
    end
    c_log( "[robotclient](on_defined_text_str) _dpid: 0x%X, text_id: %d", _dpid, text_id )
end

function on_ack_quest_heianzhimen_info( _ar, _dpid, _size )
    local quest_process_ = _ar:read_ushort()
    local current_quest_id_ = _ar:read_ulong()
end

function on_time_ctrl_consume( _ar, _dpid, _size )
    local id = _ar:read_int()
    local val = _ar:read_int()
end

function on_ack_quest_mammons_treasure_info( _ar, _dpid, _size )
    quest_mammons_treasure.unserialize_for_client( _ar )
end

function on_goblin_market_all_info( _ar, _dpid, _size )
    player_t.unserialize_goblin_market_for_client( _ar )
end

function on_pray_player_data( _ar, _dpid, _size )
    player_t.unserialize_pray_for_client( _ar )
end

function on_server_time( _ar, _dpid, _size )
    local server_time = _ar:read_int()
end

function on_main_quest_info( _ar, _dpid, _size )
    quest_main_t.unserialize_main_quest_for_client( _ar )
end

function on_login_result( _ar, _dpid, _size )
    local login_result = _ar:read_ushort()
    local account_id = _ar:read_double()
    local login_token = _ar:read_int()

    if login_result == 0 then
        local dpid_info = g_dpid_mng[_dpid]

        dpid_info.account_id_ = account_id
        dpid_info.login_token_ = login_token

        g_robotclient:c_disconnect( _dpid )

        tinsert( g_pending_certify, dpid_info )
        g_robotclient:c_connect_gamesvr()

        c_log( sformat( "[ROBOT](on_login_result)account id: %d, loginserver verify success", account_id ) )
    else
        c_err( sformat( "[ROBOT](on_login_result)account id: %d, loginserver verify failed, login_result: %d", account_id, login_result ) )
        --c_debug()
    end
end

function on_sync_add_buff( _ar, _dpid, _size )
    local sn = _ar:read_ulong()
    local id = _ar:read_ulong()
    local skill_def_id = _ar:read_ulong()
    local attr_count = _ar:read_byte()
    for i = 1, attr_count do
        local attr_index = _ar:read_byte()
        local delta_value = _ar:read_float()
    end
    local equip_attr_index = _ar:read_byte()
    local equip_delta_type = _ar:read_byte()
end

function on_sync_remove_buff( _ar, _dpid, _size )
    local buff_num = _ar:read_int()
    for i = 1, buff_num, 1 do
        local sn = _ar:read_int()
    end
end

function on_chat_msg( _ar, _dpid, _size )
	local player_id = _ar:read_ulong()
	local player_name = _ar:read_string()
	local level = _ar:read_ulong()
	local head_icon_id = _ar:read_uint()
	local msg = _ar:read_string() 
	local voice_data_length_in_seconds = _ar:read_int()
	if voice_data_length_in_seconds > 0 then
		local voice_data = _ar:read_buffer()
	end
	local channal_id = _ar:read_byte()
    c_log( "[ROBOT](on_chat_msg) player_id: %u, player_name: %s, msg: %s", player_id, player_name, msg )
end

function on_avatar_ascend_star( _ar, _dpid, _size )
    local avatar_index =  _ar:read_ubyte( )
    local star = _ar:read_byte( )
end

function on_set_ultimate_skill( _ar, _dpid, _size )
    local skill_id = _ar:read_int()
end

function on_push_mystic_normal_info( _ar, _dpid, _size )
    local hero = get_hero( _dpid )
    if not hero then return end
    hero:unserialize_mystic_normal_for_client( _ar )
end

function on_push_mystic_special_info( _ar, _dpid, _size )
    local hero = get_hero( _dpid )
    if not hero then return end
    hero:unserialize_mystic_special_for_client( _ar )
end

function on_bulletin( _ar, _dpid, _size )
    local bulletin_id = _ar:read_int()
    local bulletin_cfg = BULLETIN.cfg[bulletin_id]
    for _, param_type in ipairs( bulletin_cfg ) do
        if param_type == 1 then
            _ar:read_int()
        elseif param_type == 2 then
            _ar:read_string()
        end
    end
end

function on_set_npc_visible( _ar, _dpid, _size )
    local is_visible = _ar:read_byte()
end

function on_instance_spawn_mode( _ar, _dpid, _size )
    local mode = _ar:read_int()
end

function on_send_attend_info( _ar, _dpid, _size )
    local hero = get_hero( _dpid )
    if not hero then return end
    hero:unserialize_attend_for_client( _ar )
end

function on_daily_goals( _ar, _dpid, _size )
    local goals_num = _ar:read_int()
    for i = 1, goals_num do
        _ar:read_int()
        _ar:read_int()
        _ar:read_int()
    end
end

function on_ack_msg( _ar, _dpid, _size )
end

function on_replace( _ar, _dpid, _size )
    local scene_id = _ar:read_ulong()
    local x = _ar:read_float()
    local y = _ar:read_float()
    local hero = get_hero( _dpid )
    if not hero then
        return
    end
    
    ctrl_mng.remove_all_ctrls_except_hero( _dpid )
    if hero.instance_id_ then
        robot_instance.send_loading_succ( _dpid )
    end
    --g_timermng:c_add_timer( 20, timers.RANDOM_DO_SKILL, _dpid, hero.ctrl_id_, 10 )
end

function on_player_transform_sfx( _ar, _dpid, _size )
    local ctrl_id = _ar:read_int()
    local cnt = _ar:read_byte()
    for i = 1, cnt do
        local sfx_id = _ar:read_int()
    end
end

function on_gm_cmd_change_level( _ar, _dpid, _size )
end

function on_set_ext_param( _ar, _dpid, _size )
    local index = _ar:read_int()
    local value = _ar:read_int()
end

function on_start_count_down( _ar, _dpid, _size )
    local inst_type = _ar:read_byte()
end

function on_st_ack_battlefield_status( _ar, _dpid, _size )
    local rejoin = _ar:read_int() 
end

function on_st_state_replace( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local action  = _ar:read_int()
end 

function on_door_flag( _ar, _dpid, _size )
    local door = _ar:read_byte()
    local door_flag  = _ar:read_int()
end

function on_stop_count_down( _ar, _dpid, _size )
    _ar:read_byte()
end

function on_setpos( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local x = _ar:read_float()
    local z = _ar:read_float()
end

function on_play_cinema( _ar, _dpid, _size )
    local trigger_id = _ar:read_uint()
    local cinema_id = _ar:read_uint()
end

function on_add_sfx_on_obj( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local sfx_id = _ar:read_int()
end

function on_state_hold_end( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
end

function on_state_dazed_end( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
end

function on_state_drag( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local x = _ar:read_float()
    local z = _ar:read_float()
    local speed = _ar:read_float()
end

function on_state_drag_end( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
end

function on_mst_angle_range( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local angle_max = _ar:read_float()
    local angle_min = _ar:read_float()
end

function on_start_color_corr( _ar, _dpid, _size )
    local color_corr_id = _ar:read_int()
end

function on_stop_color_corr( _ar, _dpid, _size )
    local corr_type = _ar:read_ulong()
    local relative_id = _ar:read_ulong()
    local priority = _ar:read_int()
end

function on_cancel_cd( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local skill_id = _ar:read_int()
end

function on_leading_cancel( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
end

function on_god_mode_change( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local is_god_mode = _ar:read_byte()
end

function on_change_color_stop( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local buff_id = _ar:read_ulong()
end

function on_item_reborn_result( _ar, _dpid, _size )
    local succ = _ar:read_byte()
    local op_type = _ar:read_byte()
    local index_adjust = _ar:read_byte()
    local reborn = _ar:read_byte()
    if reborn == 1 then
        local reborn_index = _ar:read_byte()
        local attr_index = _ar:read_int()
        local attr_value = _ar:read_float()
        local process_type = _ar:read_byte()
    end
end

function on_state_pull( _ar, _dpid, _size )
    local attack_id = _ar:read_ulong()
    local target_id = _ar:read_ulong()
    local tx = _ar:read_float()
    local tz = _ar:read_float()
    local speed = _ar:read_float()
end

function on_refresh_buff( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local buff_sn = _ar:read_ulong()
end

function on_set_plane_star( _ar, _dpid, _size )
    local plane_id = _ar:read_ulong()
    local star = _ar:read_byte()
end

function on_reduce_cd( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local skill_id = _ar:read_int()
    local cd_reduce = _ar:read_int()
end

function on_silence( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local switch = _ar:read_uint()
end

function on_move_dir_offset( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local offset = _ar:read_int()
end

function on_change_skill( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local from_skill_id = _ar:read_int()
    local to_skill_id = _ar:read_int()
    local change_time = _ar:read_int()
end

function on_mst_revive( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local cnt = _ar:read_byte()
    for i = 1, cnt, i do
        local sfx_id = _ar:read_ulong()
    end
    act_id = _ar:read_ulong()
end

function on_item_rune_absorb( _ar, _dpid, _size )
    local index_adjust = _ar:read_byte()
    local rcode = _ar:read_byte()
    if rcode >= item_t.RCODE_SHOW_SUCC_AND_LEVELUP then
        local item = item_t.unserialize_item_for_client( _ar )
    end
end

function on_state_pick( _ar, _dpid, _size )
end

function on_stop_tgt_sfx_move( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local x = _ar:read_float()
    local z = _ar:read_flat()
end

function on_item_refine_ret( _ar, _dpid, _size )
    local adjusted_index = _ar:read_byte()
    local refine_mode = _ar:read_byte()
    local add_exp = _ar:read_int()
    local is_level_up = _ar:read_byte()
end

function on_kill_type( _ar, _dpid, _size )
    local attacker_obj_id = _ar:read_ulong()
    local target_obj_id = _ar:read_ulong()
    local kill_type = _ar:read_int()
    local revive_sec = _ar:read_int()
    local live_member_ctrl_id = _ar:read_int()
end

function on_strafe_direction( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local off_angle = _ar:read_float()
    local time = _ar:read_flat()
end

function on_item_stone_mount( _ar, _dpid, _size )
    local hole_index = _ar:read_byte()
    local adjust_index = _ar:read_int()
    local mount_type = _ar:read_byte()
end

function on_hook_bullet_target( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local bullet_id = _ar:read_ulong()
end

function on_pick_item( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
end

function on_use_potion_result( _ar, _dpid, _size )
    local result = _ar:read_byte()
end

function on_set_fog_color( _ar, _dpid, _size )
    local world_id = _ar:read_int()
    local color_id = _ar:read_int()
end

function on_trigger_patrol_path( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local sz = _ar:read_byte()
    if sz > 0 then
        for i = 1, sz, 1 do
            local x = _ar:read_int()
            local z = _ar:read_int()
        end
        local speed = _ar:read_int()
    end
end

function on_request_state_skill_result( _ar, _dpid, _size )
end

function on_morale_percent( _ar, _dpid, _size )
    local left_percent = _ar:read_int()
end

function on_use_revive_item( _ar, _dpid, _size )
end

function on_instance_auto_counter( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
    local max_count = _ar:read_int()
    local cur_count = _ar:read_int()
    local roll_flag = _ar:read_byte()
    local stop_type = _ar:read_byte()
    local camp_type = _ar:read_byte()
    local init_camp = _ar:read_byte()
end

function on_state_navigation_end( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
end

function on_del_hurt_states( _ar, _dpid, _size )
    local ctrl_id = _ar:read_ulong()
end

function on_item_inherit_result( _ar, _dpid, _size )
    local adjust_index = _ar:read_byte()
    local preview_mode = _ar:read_byte()
    if preview_mode == 1 then
        local item = item_t.unserialize_item_for_client( _ar )
    end
end

function on_ack_req_npc_interaction( _ar, _dpid, _size )
    local npc_id = _ar:read_int()
    local content_id = _ar:read_int()
    local can_interact_val = _ar:read_ubyte()
end

function on_equip_last_index_info( _ar, _dpid, _size )
    local last_index = _ar:read_int()
    local cur_index = _ar:read_int()
end

function on_add_item_complete( _ar, _dpid, _size )
end

function on_item_decompose_ret( _ar, _dpid, _size )
    local silver_sand = _ar:read_int()
    local golden_sand = _ar:read_int()
    local decompose_count = _ar:read_byte()
    local count = _ar:read_int()
    for i = 1, count, 1 do
        local stone_id = _ar:read_int()
        local stone_num = _ar:read_byte()
    end
    local magic_stone_num = _ar:read_int()
end

function on_item_magic_result( _ar, _dpid, _size )
    local adjust_index = _ar:read_int()
    item_t.unserialize_attr_for_client( _ar )
    local level_delta = _ar:read_int()
    local exp_delta = _ar:read_int()
    local first_time = _ar:read_byte()
end

function on_player_transform_enhance( _ar, _dpid, _size )
    local model_index = _ar:read_byte()
    local model_level = _ar:read_byte()
end

function on_ack_event_price_board_info( _ar, _dpid, _size )
    event_price_board_t.unserialize_event_price_board_for_client( _ar )
end

function on_player_rename( _ar, _dpid, _size )
    local result_code = _ar:read_int()
    if result_code == login_code.PLAYER_NAME_OK then
        local ctrl_id = _ar:read_int()
        local player_id = _ar:read_int()
        local player_name = _ar:read_string()
        local ctrl = ctrl_mng.get_ctrl( _dpid, ctrl_id )
        if ctrl and ctrl:is_player() and ctrl.player_id_ == player_id then
            ctrl.player_name_ = player_name
        end
        c_log( "[robotclient](on_player_rename) ctrl_id: %d, player_id: %d, player_name: %s, dpid: 0x%X" , ctrl_id, player_id, player_name, _dpid )
    else
        c_err( "[robotclient](on_player_rename) result_code: %d, dpid: 0x%X", result_code, _dpid )
    end
end

function on_update_recharge_today( _ar, _dpid, _size )
    local recharge_today = _ar:read_int()
end

function on_add_holiday_event_info( _ar, _dpid, _size )
    local len = _ar:read_byte()
    for i = 1, len do
        local key = _ar:read_int()
        local value = _ar:read_int()
    end
end

function on_add_holiday_value_notify( _ar, _dpid, _size )
    local event_type = _ar:read_int()
    local value = _ar:read_int()
end

function on_add_holiday_status( _ar, _dpid, _size )
    local holiday_status = _ar:read_byte()
end

function on_time_event_on( _ar, _dpid, _size )
    local value = _ar:read_int()
end

function on_init_parchment_quest_state( _ar, _dpid, _size )
    _ar:read_byte()
    _ar:read_float()
    _ar:read_float()
    _ar:read_int()
end

function on_time_limited_entrance( _ar, _dpid, _size )
    local count = _ar:read_byte()
    for i = 1, count, 1 do
        _ar:read_ubyte()
    end
end

function on_ping( _ar, _dpid, _size )
    local time = _ar:read_uint()
end

function on_send_finished_quest( _ar, _dpid, _size )
    local quest_id = _ar:read_uint()
end

function on_earl_pass_days( _ar, _dpid, _size )
    local pass_days = _ar:read_byte()
    local total_days = _ar:read_byte()
end

function on_reset_leona_gift_record( _ar, _dpid, _size )
end

function on_guild_attr( _ar, _dpid, _size )
end

function on_guild_members( _ar, _dpid, _size )
end

function on_guild_apply_list( _ar, _dpid, _size )
end

function on_guild_my_invitation_list( _ar, _dpid, _size )
end

function on_guild_my_apply_list( _ar, _dpid, _size )
end

function on_guild_operate_res( _ar, _dpid, _size )
end

function on_guild_new_invitation( _ar, _dpid, _size )
end

function on_guild_notify_msg( _ar, _dpid, _size )
end

function on_guild_my_apply_res( _ar, _dpid, _size )
end

function on_guild_kick_res( _ar, _dpid, _size )
end

function on_guild_set_role_res( _ar, _dpid, _size )
end

function on_daily_energy_ret( _ar, _dpid, _size )
end

function on_auto_recover_energy( _ar, _dpid, _size )
    local year = _ar:read_int()
    local month = _ar:read_byte()
    local day = _ar:read_byte()
    local hour = _ar:read_byte()
    local min = _ar:read_byte()
    local sec = _ar:read_byte()
    local secs = _ar:read_int()
end

function on_push_service( _ar, _dpid, _size )
end

function on_i_do_not_care( _ar, _dpid, _size )
end

function on_welfare_buy( _ar, _dpid, _size )
end

function on_buy_gold( _ar, _dpid, _size )
end

function on_notify_interaction_condition( _ar, _dpid, _size )
end

function on_sync_enter_inst_record( _ar, _dpid, _size )
end

function on_guildprof_task_info( _ar, _dpid, _size )
end

function on_update_vip_info( _ar, _dpid, _size )
end

function on_guild_donation_info( _ar, _dpid, _size )
end

function on_guild_update_equip_gs( _ar, _dpid, _size )
end

function on_update_daily_goal_level_data( _ar, _dpid, _size )
end

function on_event_calendar_today( _ar, _dpid, _size )
end

function on_event_calendar_state( _ar, _dpid, _size )
end

function on_event_calendar_coming( _ar, _dpid, _size )
end

function on_guild_daily_pvp_clear_all_data( _ar, _dpid, _size )
end

function on_set_bind_gold_max_level( _ar, _dpid, _size )
end

function on_enter_scene_completed( _ar, _dpid, _size )
end

function on_login_to_town( _ar, _dpid, _size )
end

function on_pet_update_access_data( _ar, _dpid, _size )
    local ret = _ar:read_int()
    local pet_id = _ar:read_int()
end

function on_level_reward_data( _ar, _dpid, _size )
end

function on_server_info( _ar, _dpid, _size )
    local is_debug = _ar:read_byte()
    local is_open_newbie_guild = _ar:read_byte()
    local game_area = _ar:read_string()
end

function on_add_awake_weapon_sfxs( _ar, _dpid, _size )
    local obj_id = _ar:read_ulong()
    local count = _ar:read_byte()

    for i = 1, count, 1 do
        local sfx_id = _ar:read_int()
    end

    local active_level = _ar:read_byte()
end

function on_sdklog_create_order( _ar, _dpid, _size )
    local order_id = _ar:read_string()
    local amount = _ar:read_string()
    local goods_id = _ar:read_string()
end

function on_gameserver_remote_call_miniserver_transfer( _ar, _dpid, _size )
    local func_name = _ar:read_string()
    local args = _ar:read_buffer()

    --local args_table = c_msgunpack( args )
    --func( _dpid, unpack( args_table ) )
    c_log( "[robotclient](on_gameserver_remote_call_miniserver_transfer) func_name: %s", func_name )
end

function on_weekend_pvp_team_enroll_result( _ar, _dpid, _size )
end

function on_weekend_pvp_player_enroll_state( _ar, _dpid, _size )
end

function on_weekend_pvp_cancel_team_qualification( _ar, _dpid, _size )
end

function on_weekend_pvp_battle_info( _ar, _dpid, _size ) 
end

function on_weekend_pvp_battle_result( _ar, _dpid, _size )
end

function on_weekend_pvp_event_state( _ar, _dpid, _size )
end

function on_weekend_pvp_rank_info( _ar, _dpid, _size )
end

function on_soul_contract_essence( _ar, _dpid, _size )
end

function on_soul_contract_pool( _ar, _dpid, _size )
end

function on_ares_update_info( _ar, _dpid, _size )
end

function on_update_first_time_rechage( _ar, _dpid, _size )
end

function on_clear_recharge_award( _ar, _dpid, _size )
end

function on_add_weekly_info( _ar, _dpid, _size )
end

function on_five_pvp_match_start( _ar, _dpid, _size)
end

function on_skill_secret( _ar, _dpid, _size )
    local secret = _ar:read_int()

    local dpid_info = g_dpid_mng[_dpid]

    if (dpid_info == nil) then
        dpid_info = {}
        g_dpid_mng[_dpid] = dpid_info
    end

    dpid_info.skill_secret_ = secret

end

function on_state_pick_end( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_int()
end

function on_add_chain_sfx( _ar, _dpid, _size )
    local from_obj_ctrl_id = _ar:read_ulong()
    local to_obj_ctrl_id = _ar:read_ulong()
    local sfx_id = _ar:read_int()
end

function on_state_freeze( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local attacker_id = _ar:read_ulong()
end

function on_trace_search( _ar, _dpid, _size )
    local search_type = _ar:read_int()
    local objid = _ar:read_int()
    local x = _ar:read_float()
    local z = _ar:read_float()
    local angle = _ar:read_float()
    local f1 = _ar:read_float()
    local f2 = _ar:read_float()
end

function on_state_freeze_end( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
end

function on_state_fear( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local attacker_id = _ar:read_ulong()
end

function on_state_fear_end( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local x, z, angle_y = _ar:read_compress_pos_angle()
end

function on_state_fear_change( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local x, z, angle_y = _ar:read_compress_pos_angle()
end

function on_state_dance( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local attacker_id = _ar:read_ulong()
end

function on_state_dance_end( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
end

function on_add_element_damage( _ar, _dpid, _size )
    local final_damage = _ar:read_int()
    local element_damage = _ar:read_int()
end

function on_show_pasv_ui( _ar, _dpid, _size )
    local pasv_id = _ar:read_int()
end

function on_silence_skill( _ar, _dpid, _size )
    local skill_id = _ar:read_int()
end

function on_silence_skill_end( _ar, _dpid, _size )
    local skill_id = _ar:read_int()
end

function on_del_state_skilling( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
end

function on_resist_mode( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local obj_resist_mode = _ar:read_byte()
end

function on_add_trace_sfx( _ar, _dpid, _size )
    local owner_obj_ctrl_id = _ar:read_ulong()
    local from_obj_ctrl_id = _ar:read_ulong()
    local to_obj_ctrl_id = _ar:read_ulong()
    local sfx_id = _ar:read_int()
    local sn = _ar:read_ulong()
end

function on_del_trace_sfx( _ar, _dpid, _size )
    local owner_obj_ctrl_id = _ar:read_ulong()
    local sn = _ar:read_ulong()
end

function on_serialize_skills( _ar, _dpid, _size )
    local skill_slots = {}
    local slot_cnt = _ar:read_byte()
    for slot = 1, slot_cnt do
        local skill_id = _ar:read_int()
        tinsert( skill_slots, skill_id )
    end
    player_t.unserialize_skill_talent_for_client( _ar )
end

function on_stop_rush( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local x = _ar:read_float()
    local z = _ar:read_float()
end

function on_state_caught( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local attacker_id = _ar:read_ulong()
    local cfg_id = _ar:read_ulong()
end

function on_state_caught_end( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
end

function on_trap_camp_info( _ar, _dpid, _size )
    local owner_trigger_ctrl_id = _ar:read_ulong()
    local guild_server_id = _ar:read_int()
    local guild_id = _ar:read_int()
    local attacker_wild_role = _ar:read_byte()
    local attacker_resist_mode = _ar:read_byte()
end

function on_bullet_target_id( _ar, _dpid, _size )
    local bullet_ctrl_id = _ar:read_ulong()
    local target_id = _ar:read_ulong()
    local turn_speed = _ar:read_float()
end

function on_set_only_attacker( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local obj_only_attacker_id = _ar:read_ulong()
end

function on_ack_treasure_hunt_composite_map( _ar, _dpid, _size )
    local item_id = _ar:read_int()
    local item_num = _ar:read_int()
end

function on_preload_missile( _ar, _dpid, _size )
    local cfg_id = _ar:read_ulong()
end

function on_refuse_reborn( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
end

function on_reborn_params( _ar, _dpid, _size )
    local hp_percent = _ar:read_int()
    local timeout_ms = _ar:read_int()
end

function on_missile_stage( _ar, _dpid, _size )
    local missile_ctrl_id = _ar:read_int()
    local stage_index = _ar:read_int()
    local stage_type = _ar:read_int()
    local sx = _ar:read_float()
    local sy = _ar:read_float()
    local sz = _ar:read_float()
    local start_angle_y = _ar:read_float()
end

function on_predict_missile_stage_end( _ar, _dpid, _size )
    local missile_ctrl_id = _ar:read_int()
    local stage_index = _ar:read_int()
    local persist_time = _ar:read_float()
    local end_x = _ar:read_float()
    local end_y = _ar:read_float()
    local end_z = _ar:read_float()
    local end_angle_y = _ar:read_float()
end

function on_missile_target( _ar, _dpid, _size )
    local missile_ctrl_id = _ar:read_ulong()
    local target_id = _ar:read_ulong()
end

function on_missile_delete_info( _ar, _dpid, _size )
    local missile_ctrl_id = _ar:read_ulong()
    local lifetime_in_ms = _ar:read_int()
    local stage_index = _ar:read_int()
end

function on_reset_all_skill_cd( _ar, _dpid, _size )

end

function on_reset_all_job_skill_cd( _ar, _dpid, _size )

end

function on_avatar_destroy( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local item_list_cnt = _ar:read_ubyte()
    for i = 1, item_list_cnt do
        local item1 = _ar:read_int()
        local item2 = _ar:read_int()
    end
    local destroy_free_cnt = _ar:read_ubyte()
end

function on_avatar_mat_conversion( _ar, _dpid, _size )
    local item_id = _ar:read_int()
    local item_num = _ar:read_int()
end

function on_avatar_ascend_level( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local exp = _ar:read_int()
end

function on_avatar_med_ascend_star( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local med_index = _ar:read_ubyte()
    local star = _ar:read_ubyte()
end

function on_avatar_med_reborn_skill( _ar, _dpid, _size )
    local avatar_index = _ar:read_ubyte()
    local med_index = _ar:read_ubyte()
    local item_index = _ar:read_ubyte()
    local skill = _ar:read_int()
    local attrs_cnt = _ar:read_ubyte()
    for i = 1, attr_cnt do
        local attr = _ar:read_int()
    end
end

-- ST_CLEAR_ALL_SYNCED_BUFF not found or empty!
function on_clear_all_synced_buff( _ar, _dpid, _size )

end

-- ST_PVP_SCORE_UPDATE
function on_pvp_score_update( _ar, _dpid, _size )
    local red_score = _ar:read_int()
    local blue_score = _ar:read_int() 
end

-- ST_ACK_INSTANCE_SWEEP
function on_ack_instance_sweep( _ar, _dpid, _size )
    local err_code = _ar:read_byte()
    if err_code > 0 then return end

    local act_inst_id = _ar:read_int()
    local gold = _ar:read_int()
    local exp = _ar:read_int()
    local level_up = _ar:read_ubyte()
    local new_level = _ar:read_int()
    local is_send_mail = _ar:read_ubyte()
    local enter_type = _ar:read_ubyte()
    local diamond_card_rate = _ar:read_float()
    local holiday_double_rate = _ar:read_float()
    local drop_param_instance_base_exp = _ar:read_float()
    local drop_param_activity_exp = _ar:read_float()
    local witt_rate = _ar:read_float() 
    local train_camp_rate = _ar:read_float() 
    local drop_param_guild_add_percent = _ar:read_byte()
    local drop_param_guild_prof_exp_percent = _ar:read_byte()
    local drop_param_guild_prof_gold_percent = _ar:read_byte()
    local drop_param_guild_prof_glory_percent = _ar:read_byte()
    local drop_param_guild_prof_honor_percent = _ar:read_byte()

    local items_cnt = _ar:read_int()
    for i = 1, items_cnt do
        drop_item_t.unserialize( _ar )
    end
end

-- ST_WEEKEND_PVP_MIGRATE_NOTIFY
function on_weekend_pvp_migrate_notify( _ar, _dpid, _size )
    local can_join = _ar:read_ubyte()
    local show_message = _ar:read_ubyte() 
end

-- ST_RECONNECT_DONE not found or empty!
function on_reconnect_done( _ar, _dpid, _size )

end

-- ST_BUY_ITEM_STONE_RESULT
function on_buy_item_stone_result( _ar, _dpid, _size )
    local stone_id = _ar:read_int()
    local stone_num = _ar:read_int()
    local result = _ar:read_byte()
end

-- ST_ORANGE_EQUIP_UPGRADE
function on_orange_equip_upgrade( _ar, _dpid, _size )
    local index = _ar:read_int()
    local delta_list_cnt = _ar:read_byte()
    for i = 1, delta_list_cnt do
        local v = _ar:read_int()
    end
end

-- ST_ORANGE_EQUIP_EXCHANGE not found or empty!
function on_orange_equip_exchange( _ar, _dpid, _size )

end

-- ST_UPDATE_WAREHOUSE_ITEM
function on_update_warehouse_item( _ar, _dpid, _size )
    local self_ctrl_id = _ar:read_ulong()
    local index = _ar:read_int()
    item_t.unserialize_item_for_client( _ar )
end

-- ST_ADD_WAREHOUSE_ITEM
function on_add_warehouse_item( _ar, _dpid, _size )
    local self_ctrl_id = _ar:read_ulong()
    item_t.unserialize_item_for_client( _ar )
end

-- ST_REMOVE_WAREHOUSE_ITEM
function on_remove_warehouse_item( _ar, _dpid, _size )
    local self_ctrl_id = _ar:read_ulong()
    local index = _ar:read_int()
end

-- ST_BOOK_DECOMPOSE_RET
function on_book_decompose_ret( _ar, _dpid, _size )
    local book_count = _ar:read_byte()
    local count = _ar:read_byte()
    for i = 1, count do
        local id = _ar:read_int()
        local num = _ar:read_int()
    end
end

-- ST_BOOK_COMPOUND_RET
function on_book_compound_ret( _ar, _dpid, _size )
    local book_id = _ar:read_int()
end

-- ST_GUILD_DAILY_PVP_SET_CHARACTER
function on_guild_daily_pvp_set_character( _ar, _dpid, _size )
    local character_code = _ar:read_ubyte()
end

-- ST_ONE_KEY_EQUIP
function on_one_key_equip( _ar, _dpid, _size )
    local ret = _ar:read_int()
end

-- ST_TRANSFORM_LEARN_SKILL not found or empty!
function on_transform_learn_skill( _ar, _dpid, _size )

end

-- ST_TRANSFORM_ENHANCE_SKILL not found or empty!
function on_transform_enhance_skill( _ar, _dpid, _size )

end

-- ST_TRANSFORM_DISCARD_SKILL not found or empty!
function on_transform_discard_skill( _ar, _dpid, _size )

end

-- ST_TRANSFORM_ENHANCE_SKILL_FAIL not found or empty!
function on_transform_enhance_skill_fail( _ar, _dpid, _size )

end

-- ST_GUILD_EXCHANGE_ELEMENT_ACK
function on_guild_exchange_element_ack( _ar, _dpid, _size )
    local item_id = _ar:read_int()
    local num = _ar:read_int()
end

-- ST_ACK_LOADING_SUCCESS not found or empty!
function on_ack_loading_success( _ar, _dpid, _size )

end

-- ST_BUY_ORANGE_RESULT
function on_buy_orange_result( _ar, _dpid, _size )
    local orange_num = _ar:read_int()
    local result = _ar:read_byte()
end

-- ST_ORANGE_EQUIP_AWAKE_RESULT
function on_orange_equip_awake_result( _ar, _dpid, _size )
    local result = _ar:read_byte()
    local adjust_index = _ar:read_int()
end

-- ST_ORANGE_MELT_RESULT
function on_orange_melt_result( _ar, _dpid, _size )
    local left_num = _ar:read_int()
    local adjust_index = _ar:read_int()
end

-- ST_SET_MONSTER_SKILL_ANGLE
function on_set_monster_skill_angle( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_int()
    local angle = _ar:read_float()
end

-- ST_DO_MONSTER_CHASE_SKILL_MOVE
function on_do_monster_chase_skill_move( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_int()
    local speed = _ar:read_float()

    local pos_count = _ar:read_ubyte()
    for i = 1, pos_count do
        local pos1_10 = _ar:read_ushort()
        local pos2_10 = _ar:read_ushort()
    end
end

-- ST_CORR_POS_SKILL
function on_corr_pos_skill( _ar, _dpid, _size )
    local obj_ctrl_id = _ar:read_ulong()
    local x, y, z = obj_core_:c_getpos()
    local x, z, angle_y = _ar:read_compress_pos_angle()
end

-- s/ar:write_\(\w*\)( *_*\(.\{-}\)_* *)/local \2 = _ar:read_\1()
func_table =
{
    [msg.PT_DISCONNECT] = on_disconnect,
    [msg.ST_STATE_PICK_END] = on_state_pick_end,
    [msg.ST_ADD_CHAIN_SFX] = on_add_chain_sfx,
    [msg.ST_STATE_FREEZE] = on_state_freeze,
    [msg.ST_TRACE_SEARCH] = on_trace_search,
    [msg.ST_STATE_FREEZE_END] = on_state_freeze_end,
    [msg.ST_STATE_FEAR] = on_state_fear,
    [msg.ST_STATE_FEAR_END] = on_state_fear_end,
    [msg.ST_STATE_FEAR_CHANGE] = on_state_fear_change,
    [msg.ST_STATE_DANCE] = on_state_dance,
    [msg.ST_STATE_DANCE_END] = on_state_dance_end,
    [msg.ST_ADD_ELEMENT_DAMAGE] = on_add_element_damage,
    [msg.ST_SHOW_PASV_UI] = on_show_pasv_ui,
    [msg.ST_SILENCE_SKILL] = on_silence_skill,
    [msg.ST_SILENCE_SKILL_END] = on_silence_skill_end,
    [msg.ST_DEL_STATE_SKILLING] = on_del_state_skilling,
    [msg.ST_DEL_STATE_DAZED] = on_del_state_dazed,
    [msg.ST_RESIST_MODE] = on_resist_mode,
    [msg.ST_ADD_TRACE_SFX] = on_add_trace_sfx,
    [msg.ST_DEL_TRACE_SFX] = on_del_trace_sfx,
    [msg.ST_SERIALIZE_SKILLS] = on_serialize_skills,
    [msg.ST_STOP_RUSH] = on_stop_rush,
    [msg.ST_STATE_CAUGHT] = on_state_caught,
    [msg.ST_STATE_CAUGHT_END] = on_state_caught_end,
    [msg.ST_TRAP_CAMP_INFO] = on_trap_camp_info,
    [msg.ST_BULLET_TARGET_ID] = on_bullet_target_id,
    [msg.ST_SET_ONLY_ATTACKER] = on_set_only_attacker,
    [msg.ST_ACK_TREASURE_HUNT_COMPOSITE_MAP] = on_ack_treasure_hunt_composite_map,
    [msg.ST_PRELOAD_MISSILE] = on_preload_missile,
    [msg.ST_REFUSE_REBORN] = on_refuse_reborn,
    [msg.ST_REBORN_PARAMS] = on_reborn_params,
    [msg.ST_MISSILE_STAGE] = on_missile_stage,
    [msg.ST_PREDICT_MISSILE_STAGE_END] = on_predict_missile_stage_end,
    [msg.ST_MISSILE_TARGET] = on_missile_target,
    [msg.ST_MISSILE_DELETE_INFO] = on_missile_delete_info,
    [msg.ST_RESET_ALL_SKILL_CD] = on_reset_all_skill_cd,
    [msg.ST_RESET_ALL_JOB_SKILL_CD] = on_reset_all_job_skill_cd,
    [msg.ST_AVATAR_DESTROY] = on_avatar_destroy,
    [msg.ST_AVATAR_MAT_CONVERSION] = on_avatar_mat_conversion,
    [msg.ST_AVATAR_ASCEND_LEVEL] = on_avatar_ascend_level,
    [msg.ST_AVATAR_MED_ASCEND_STAR] = on_avatar_med_ascend_star,
    [msg.ST_AVATAR_MED_REBORN_SKILL] = on_avatar_med_reborn_skill,
    [msg.ST_CLEAR_ALL_SYNCED_BUFF] = on_clear_all_synced_buff,
    [msg.ST_PVP_SCORE_UPDATE] = on_pvp_score_update,
    [msg.ST_ACK_INSTANCE_SWEEP] = on_ack_instance_sweep,
    [msg.ST_WEEKEND_PVP_MIGRATE_NOTIFY] = on_weekend_pvp_migrate_notify,
    [msg.ST_RECONNECT_DONE] = on_reconnect_done,
    [msg.ST_BUY_ITEM_STONE_RESULT] = on_buy_item_stone_result,
    [msg.ST_ORANGE_EQUIP_UPGRADE] = on_orange_equip_upgrade,
    [msg.ST_ORANGE_EQUIP_EXCHANGE] = on_orange_equip_exchange,
    [msg.ST_UPDATE_WAREHOUSE_ITEM] = on_update_warehouse_item,
    [msg.ST_ADD_WAREHOUSE_ITEM] = on_add_warehouse_item,
    [msg.ST_REMOVE_WAREHOUSE_ITEM] = on_remove_warehouse_item,
    [msg.ST_BOOK_DECOMPOSE_RET] = on_book_decompose_ret,
    [msg.ST_BOOK_COMPOUND_RET] = on_book_compound_ret,
    [msg.ST_GUILD_DAILY_PVP_SET_CHARACTER] = on_guild_daily_pvp_set_character,
    [msg.ST_ONE_KEY_EQUIP] = on_one_key_equip,
    [msg.ST_TRANSFORM_LEARN_SKILL] = on_transform_learn_skill,
    [msg.ST_TRANSFORM_ENHANCE_SKILL] = on_transform_enhance_skill,
    [msg.ST_TRANSFORM_DISCARD_SKILL] = on_transform_discard_skill,
    [msg.ST_TRANSFORM_ENHANCE_SKILL_FAIL] = on_transform_enhance_skill_fail,
    [msg.ST_GUILD_EXCHANGE_ELEMENT_ACK] = on_guild_exchange_element_ack,
    [msg.ST_ACK_LOADING_SUCCESS] = on_ack_loading_success,
    [msg.ST_BUY_ORANGE_RESULT] = on_buy_orange_result,
    [msg.ST_ORANGE_EQUIP_AWAKE_RESULT] = on_orange_equip_awake_result,
    [msg.ST_ORANGE_MELT_RESULT] = on_orange_melt_result,
    [msg.ST_SET_MONSTER_SKILL_ANGLE] = on_set_monster_skill_angle,
    [msg.ST_DO_MONSTER_CHASE_SKILL_MOVE] = on_do_monster_chase_skill_move,
    [msg.ST_CORR_POS_SKILL] = on_corr_pos_skill,
    [msg.LC_TYPE_CREATE_ACCNT_RESULT] = on_create_account_result,
    [msg.LC_TYPE_LOGIN_RESULT] = on_login_result,
    [msg.ST_CERTIFY_RESULT] = on_game_certify_result,
    [msg.ST_PLAYERLIST_RESULT] = on_game_player_list,
    [msg.ST_RANDOM_NAME] = on_random_name,
    [msg.ST_CREATE_PLAYER_RESULT] = on_create_player_result,
    [msg.ST_DELETE_PLAYER_RESULT] = on_game_delete_player_result,
    [msg.ST_JOIN_PLAYER_RESULT] = on_game_join_player_result,
    [msg.ST_LOGIN_OTHER_PLACE] = on_game_login_other_place,
    [msg.ST_ALREADY_LOGIN_OTHER] = on_game_already_login_other,
    [msg.ST_LOGIN_QUEUE_FULL] = on_login_queue_full,
    [msg.ST_LOGIN_QUEUE_ENQUEUE] = on_login_queue_enqueue,
    [msg.ST_LOGIN_QUEUE_RANK] = on_login_queue_rank,
    [msg.ST_LOGIN_QUEUE_QUIT_RET] = on_login_queue_quit_ret,
    [msg.ST_ADD_OBJ] = on_add_obj,
    [msg.ST_RM_OBJ] = on_rm_obj,
    [msg.ST_SNAPSHOT] = on_game_snapshot,
    [msg.ST_SPAWN_FINISH] = on_spawn_finish,
    [msg.ST_STATE_MOVE_TO_POS] = on_move_to_pos,
    [msg.ST_STATE_MOVE_GROUND] = on_move_ground,
    [msg.ST_SET_ANGLE] = on_set_angle,
    [msg.ST_CORR_POS] = on_corr_pos,
    [msg.ST_STATE_STAND] = on_state_stand,
    [msg.ST_TRACE_POS] = on_trace_pos,
    [msg.ST_SET_CAMP] = on_set_camp,
    [msg.ST_ADD_DAMAGE] = on_add_damage,
    [msg.ST_STATE_DEAD] = on_state_dead,
    [msg.ST_ADD_BUFF] = on_add_buff,
    [msg.ST_REMOVE_BUFF] = on_rm_buff,
    [msg.ST_SKILL_JUMP] = on_skill_jump,
    [msg.ST_STATE_HURT] = on_state_hurt,
    [msg.ST_STATE_NAVIGATE] = on_state_navigate, 
    [msg.ST_SET_VALUE] = on_set_value,
    [msg.ST_STATE_SKILL] = on_state_skill,
    [msg.ST_STATE_DAZED] = on_state_dazed,
    [msg.ST_STATE_HOLD] = on_state_hold,
	[msg.ST_TRIGGER_STATE] = on_trigger_state,
	[msg.ST_TRIGGER_ACTION] = on_trigger_action,
    [msg.ST_STATE_MST_NAVIGATE]  = on_state_mst_nav,
    [msg.ST_STATE_MST_STAND] = on_state_msg_stand,
    [msg.ST_MST_ATTACKABLE] = on_mst_attackable,
    [msg.ST_STATE_RUSH] = on_state_rush,
    [msg.ST_STATE_JUMP] = on_state_jump,
    [msg.ST_MOVE_PERSIST_CANCEL] = on_move_persist_cancel,
    [msg.ST_STATE_MOVE_PERSIST_PAUSE] = on_state_move_persist_pause,
    [msg.ST_TRACE_STATE] = on_trace_state,
    [msg.ST_ADD_SFX] = on_add_sfx,
    [msg.ST_CHANGE_COLOR] = on_change_color,
    [msg.ST_CHANGE_SCALE] = on_change_scale,
    [msg.ST_REVIVAL] = on_revival,
    [msg.ST_EQUIP] = on_add_equip_info,
    [msg.ST_UNLOCK_JOB_SKILLS] = on_unlock_job_skills,
    [msg.ST_UNLOCKED_TALENTS] = on_unlocked_talents,
    [msg.ST_LEVEL_UP] = on_level_up,
    [msg.ST_ADD_ITEM] = on_add_item,
    [msg.ST_REMOVE_ITEM] = on_remove_item,
    [msg.ST_UPDATE_ITEM] = on_update_item,
    [msg.ST_REMOVE_ITEM_COMPLETE] = on_remove_item_complete,
    [msg.ST_SET_JOB_SKILL] = on_set_job_skill,
    [msg.ST_MIGRATE_BF_BEGIN] = on_migrate_bf_begin,
    [msg.ST_ACTION_SFX] = on_action_sfx,
    [msg.ST_CORR_POS_HERO] = on_corr_pos_hero,
    [msg.ST_REJOIN_GAME_BEGIN] = on_rejoin_game_begin,
    [msg.ST_TIME_CTRL_REFRESH] = on_time_ctrl_refresh,
    [msg.ST_ACK_QUEST_WORLD_MAP_INFO] = on_ask_quest_world_map_info,
    [msg.ST_STATE_CINEMA] = on_state_cinema,
    [msg.ST_ADD_MAIL] = on_add_mail,
    [msg.ST_DEFINED_TEXT] = on_defined_text,
    [msg.ST_DEFINED_TEXT_STR] = on_defined_text_str,  
  --  [msg.ST_DEFINED_TEXT_STR] = on_i_do_not_care,
    [msg.ST_ACK_QUEST_HEIANZHIMEN_INFO] = on_ack_quest_heianzhimen_info,
    [msg.ST_TIME_CTRL_CONSUME] = on_time_ctrl_consume,
    [msg.ST_ACK_QUEST_MAMMONS_TREASURE_INFO] = on_ack_quest_mammons_treasure_info,
    [msg.ST_GOBLIN_MARKET_ALL_INFO] = on_goblin_market_all_info,
    [msg.ST_PRAY_PLAYER_DATA] = on_pray_player_data,
    [msg.ST_SERVER_TIME] = on_server_time,
    [msg.ST_MAIN_QUEST_INFO] = on_main_quest_info,
    [msg.ST_SYNC_ADD_BUFF] = on_sync_add_buff,
    [msg.ST_SYNC_REMOVE_BUFF] = on_sync_remove_buff,
    [msg.ST_AVATAR_ASCEND_STAR] = on_avatar_ascend_star,
    [msg.ST_SET_ULTIMATE_SKILL] = on_set_ultimate_skill,
    [msg.ST_PUSH_MYSTIC_NORMAL_INFO] = on_push_mystic_normal_info,
    [msg.ST_PUSH_MYSTIC_SPECIAL_INFO] = on_push_mystic_special_info,
    [msg.ST_BULLETIN] = on_bulletin,
    [msg.ST_SET_NPC_VISIBLE] = on_set_npc_visible,
    [msg.ST_SHOP] = robot_shop.on_shop,
    [msg.ST_SHOP_LIST] = robot_shop.on_shop_list,
    [msg.ST_SHOP_OPERATE_FAIL] = robot_shop.on_shop_operate_fail,
    [msg.ST_MY_SHOPS] = robot_shop.on_my_shops,
    [msg.ST_SHOP_BID_GOLD_INFO] = robot_shop.on_shop_bid_gold_info,
    [msg.ST_INSTANCE_SPAWN_MODE] = on_instance_spawn_mode,
    [msg.ST_ADD_ATTEND_INFO] = on_send_attend_info,
    [msg.ST_DAILY_GOALS] = on_daily_goals,
    [msg.ST_PRELOAD_DROP_ITEM_MODEL] = robot_instance.on_preload_drop_item_model,
    [msg.SM_CREATE_INSTANCE] = robot_instance.on_miniserver_create_instance,
    [msg.ST_START_INSTANCE_SPAWNING] = robot_instance.on_start_instance_spawning,
    [msg.ST_END_INSTANCE_SPAWNING] = robot_instance.on_end_instance_spawning,
    [msg.ST_ACK_MSG] = on_ack_msg,
    [msg.ST_JOIN_INSTANCE_SUCC] = robot_instance.on_join_instance_succ,
    [msg.ST_JOIN_INSTANCE_FAIL] = robot_instance.on_join_instance_fail,
    [msg.ST_QUIT_INSTANCE_SUCC] = robot_instance.on_quit_instance_succ,
    [msg.ST_REPLACE] = on_replace,
    -- [msg.ST_PLAYER_TRANSFORM_SFX] = on_player_transform_sfx,
    [msg.ST_GM_CMD_CHANGE_LEVEL] = on_gm_cmd_change_level,
    [msg.ST_SET_EXT_PARAM] = on_set_ext_param,
    [msg.ST_START_COUNT_DOWN] = on_start_count_down,
    [msg.ST_ACK_BATTLEFIELD_STATUS] = on_st_ack_battlefield_status,
    [msg.ST_STATE_REPLACE] = on_st_state_replace,
    [msg.ST_DOOR_FLAG] = on_door_flag,
    [msg.ST_STOP_COUNT_DOWN] = on_stop_count_down,
    [msg.ST_SETPOS] = on_setpos,
    [msg.ST_PLAY_CINEMA] = on_play_cinema,
    [msg.ST_QUIT_INSTANCE] = robot_instance.on_quit_instance,
    [msg.ST_ADD_SFX_ON_OBJ] = on_add_sfx_on_obj,
    [msg.ST_STATE_HOLD_END] = on_state_hold_end,
    [msg.ST_STATE_DAZED_END] = on_state_dazed_end,
    [msg.ST_STATE_DRAG] = on_state_drag,
    [msg.ST_STATE_DRAG_END] = on_state_drag_end,
    [msg.ST_MST_ANGLE_RANGE] = on_mst_angle_range,
    [msg.ST_START_COLOR_CORR] = on_start_color_corr,
    [msg.ST_STOP_COLOR_CORR] = on_stop_color_corr,
    [msg.ST_CANCEL_CD] = on_cancel_cd,
    [msg.ST_LEADING_CANCEL] = on_leading_cancel,
    [msg.ST_GOD_MODE_CHANGE] = on_god_mode_change,
    [msg.ST_CHANGE_COLOR_STOP] = on_change_color_stop,
    [msg.ST_ITEM_REBORN_RESULT] = on_item_reborn_result,
    [msg.ST_STATE_PULL] = on_state_pull,
    [msg.ST_END_MANUAL_UP_LEVEL] = on_end_manual_up_level,
    [msg.ST_REFRESH_BUFF] = on_refresh_buff,
    [msg.ST_SET_PLANE_STAR] = on_set_plane_star,
    [msg.ST_REDUCE_CD] = on_reduce_cd,
    [msg.ST_SILENCE] = on_silence,
    [msg.ST_MOVE_DIR_OFFSET] = on_move_dir_offset,
    [msg.ST_CHANGE_SKILL] = on_change_skill,
    [msg.ST_PLANE_DROP] = robot_instance.on_plane_drop,
    [msg.ST_MST_REVIVE] = on_mst_revive,
    [msg.ST_ITEM_RUNE_ABSORB] = on_item_rune_absorb,
    [msg.ST_STATE_PICK] = on_state_pick,
    [msg.ST_STOP_TGT_SFX_MOVE] = on_stop_tgt_sfx_move,
    [msg.ST_ITEM_REFINE_RET] = on_item_refine_ret,
    [msg.ST_KILL_TYPE] = on_kill_type,
    [msg.ST_STRAFE_DIRECTION] = on_strafe_direction,
    [msg.ST_ITEM_STONE_MOUNT] = on_item_stone_mount,
    [msg.ST_HOOK_BULLET_TARGET] = on_hook_bullet_target,
    [msg.ST_PICK_ITEM] = on_pick_item,
    [msg.ST_USE_POTION_RESULT] = on_use_potion_result,
    [msg.ST_SET_FOG_COLOR] = on_set_fog_color,
    [msg.ST_TRIGGER_PATROL_PATH] = on_trigger_patrol_path,
    [msg.ST_REQUEST_STATE_SKILL_RESULT] = on_request_state_skill_result,
    [msg.ST_MORALE_PERCENT] = on_morale_percent,
    [msg.ST_USE_REVIVE_ITEM] = on_use_revive_item,
    [msg.ST_INSTANCE_AUTO_COUNTER] = on_instance_auto_counter,
    [msg.ST_STATE_NAVIGATION_END] = on_state_navigation_end,
    [msg.ST_DEL_HURT_STATES] = on_del_hurt_states,
    [msg.ST_ITEM_INHERIT_RESULT] = on_item_inherit_result,
    [msg.ST_ACK_REQ_NPC_INTERACTION] = on_ack_req_npc_interaction,
    [msg.ST_EQUIP_LAST_INDEX_INFO] = on_equip_last_index_info,
    [msg.ST_ADD_ITEM_COMPLETE] = on_add_item_complete,
    [msg.ST_ITEM_DECOMPOSE_RET] = on_item_decompose_ret,
    [msg.ST_ITEM_MAGIC_RESULT] = on_item_magic_result,
    [msg.ST_PLAYER_TRANSFORM_ENHANCE] = on_player_transform_enhance,
    [msg.ST_ACK_EVENT_PRICE_BOARD_INFO] = on_ack_event_price_board_info,
    [msg.ST_PLAYER_RENAME] = on_player_rename,
    [msg.ST_UPDATE_RECHARGE_TODAY] = on_update_recharge_today,
    [msg.ST_ADD_HOLIDAY_EVENT_INFO] = on_add_holiday_event_info,
    [msg.ST_ADD_HOLIDAY_VALUE_NOTIFY] = on_add_holiday_value_notify,
    [msg.ST_ADD_HOLIDAY_STATUS] = on_add_holiday_status,
    [msg.ST_TIME_EVENT_ON] = on_time_event_on,
    [msg.ST_INIT_PARCHMENT_QUEST_STATE] = on_init_parchment_quest_state,
    [msg.ST_TIME_LIMITED_ENTRANCE] = on_time_limited_entrance,
    [msg.ST_PING] = on_ping,
    [msg.ST_SEND_FINISHED_QUEST] = on_send_finished_quest,
    [msg.ST_EARL_PASS_DAYS] = on_earl_pass_days,
    [msg.ST_RESET_LEONA_GIFT_RECORD] = on_reset_leona_gift_record, 
    [msg.ST_TEAM_INVITE_MSG] = robot_team.on_team_invite_msg, 
    [msg.ST_TEAM_DATA] = robot_team.on_got_team_data, 
    [msg.ST_TEAM_SERVER_ACTION] = robot_team.on_team_server_action, 
    [msg.ST_RESET_LEONA_GIFT_RECORD] = on_reset_leona_gift_record,
    [msg.SM_TYPE_RPC] = on_gameserver_remote_call_miniserver_transfer,
    [msg.ST_START_ULTIMATE_SKILL_CD] = robot_instance.on_start_ultimate_skill_cd,
    [msg.ST_PLAY_MUSIC] = robot_instance.on_play_music,
    [msg.ST_TEAM_CONFIRM_MSG] = robot_team.on_confirm_action, 
    [msg.ST_TEAM_ATTRIBUTE_CHANGED] = robot_team.member_changed_attribute, 
    [msg.ST_TEAM_MATCH_FAIL_RESULT] = robot_team.on_team_match_fail, 
    [msg.ST_TEAM_SET_STATE] = robot_team.on_team_set_state, 
    [msg.ST_TEAM_MATCH_RESULT] = robot_team.on_team_match_state,
    [msg.ST_DAILY_ENERGY_RET] = on_daily_energy_ret,
    [msg.ST_AUTO_RECOVER_ENERGY] = on_auto_recover_energy,
    [msg.LC_TYPE_PUSH_SERVICE] = on_push_service,
    [msg.ST_WEEKEND_PVP_TEAM_ENROLL_RESULT] = on_weekend_pvp_team_enroll_result,
    [msg.ST_WEEKEND_PVP_PLAYER_ENROLL_STATE] = on_weekend_pvp_player_enroll_state,
    [msg.ST_WEEKEND_PVP_CANCEL_TEAM_QUALIFICATION] = on_weekend_pvp_cancel_team_qualification,
    [msg.ST_WEEKEND_PVP_BATTLE_INFO] = on_weekend_pvp_battle_info,
    [msg.ST_WEEKEND_PVP_BATTLE_RESULT] = on_weekend_pvp_battle_result,
    [msg.ST_WEEKEND_PVP_EVENT_STATE] = on_weekend_pvp_event_state,
    [msg.ST_WEEKEND_PVP_RANK_INFO] = on_weekend_pvp_rank_info,
    --[msg.ST_SOUL_CONTRACT_ESSENCE] = on_soul_contract_essence,
    --[msg.ST_SOUL_CONTRACT_POOL] = on_soul_contract_pool,
    [msg.ST_ARES_UPDATE_INFO] = on_ares_update_info,

    [msg.ST_UPDATE_DAILY_FIRST_TIME_RECHARGE] = on_update_first_time_rechage,
    [msg.ST_CLEAR_RECHARGE_AWARD_DIAMOND] = on_clear_recharge_award,
    [msg.ST_ADD_WEEKLY_TASK_INFO] = on_add_weekly_info,

    [msg.ST_FIVE_PVP_MATCH_START] = on_five_pvp_match_start,

    [msg.ST_WELFARE_BUY] = on_welfare_buy,
    [msg.ST_BUY_GOLD] = on_buy_gold,
    [msg.ST_NOTIFY_INTERACTION_CONDITION] = on_notify_interaction_condition,
    [msg.ST_SYN_ENTER_INST_RECORD] = on_sync_enter_inst_record,
    [msg.ST_GUILDPROF_TASK_INFO] = on_guildprof_task_info,
    [msg.ST_UPDATE_VIP_INFO] = on_update_vip_info,
    [msg.ST_GUILD_DONATION_INFO] = on_guild_donation_info,
    [msg.ST_GUILD_UPDATE_EQUIP_GS] = on_guild_update_equip_gs,
    [msg.ST_UPDATE_DAILY_GOAL_LEVEL_DATA] = on_update_daily_goal_level_data,
    [msg.ST_EVENT_CALENDAR_TODAY] = on_event_calendar_today,
    [msg.ST_EVENT_CALENDAR_STATE] = on_event_calendar_state,
    [msg.ST_EVENT_CALENDAR_COMING] = on_event_calendar_coming,
    [msg.ST_GUILD_DAILY_PVP_CLEAR_ALL_DATA] = on_guild_daily_pvp_clear_all_data,
    [msg.ST_SET_BIND_GOLD_MAX_LEVEL] = on_set_bind_gold_max_level,
    [msg.ST_ENTER_SCENE_COMPLETED] = on_enter_scene_completed,
    [msg.ST_LOGIN_TO_TOWN] = on_login_to_town,
    [msg.ST_PET_UPDATE_ACCESS_DATA] = on_pet_update_access_data,
    [msg.ST_LEVEL_REWARD_DATA] = on_level_reward_data,
    [msg.ST_SERVER_INFO] = on_server_info,
    [msg.ST_ADD_AWAKE_WEAPON_SFXS] = on_add_awake_weapon_sfxs,

    [msg.ST_TYPE_BEGIN] = on_i_do_not_care,
    [msg.ST_RPC] = on_i_do_not_care,
    [msg.ST_ACTIVE_NPC_ATTR] = on_i_do_not_care,
    [msg.ST_RAW_SET_ANGLE] = on_i_do_not_care,
    [msg.ST_MOVE_PERSIST_TIME] = on_i_do_not_care,
    [msg.ST_RAW_CHANGE_COLOR] = on_i_do_not_care,
    [msg.ST_RESET_COLOR] = on_i_do_not_care,
    [msg.ST_CLIENT_CREATE_MONSTER] = on_i_do_not_care,
    [msg.ST_SET_LOGIC_MASK] = on_i_do_not_care,
    [msg.ST_SET_GAME_MODE] = on_i_do_not_care,
    [msg.ST_SHOW_BATTLE_TIPS] = on_i_do_not_care,
    [msg.ST_HIDE_BATTLE_TIPS] = on_i_do_not_care,
    [msg.ST_SERIALIZE_STATE] = on_i_do_not_care,
    [msg.ST_MANUAL_UP_LEVEL_ACK] = on_i_do_not_care,
    [msg.ST_BATTLEFIELD_MATCH_RESULT] = on_i_do_not_care,
    [msg.ST_CHAT_MSG] = on_i_do_not_care,
    [msg.ST_RPC_CSLIGHT] = on_i_do_not_care,
    [msg.ST_SHOW_BRIEF_TIPS] = on_i_do_not_care,
    [msg.ST_CHAT_PRIVATE_ECHO] = on_i_do_not_care,
    [msg.ST_BATTLEFIELD_TIPS] = on_i_do_not_care,
    [msg.ST_CRYSTAL_UNDER_ATTACK] = on_i_do_not_care,
    [msg.ST_CRYSTAL_OCCUPIED] = on_i_do_not_care,
    [msg.ST_BATTLEFIELD_OVER] = on_i_do_not_care,
    [msg.ST_MIGRATE_BF_FAIL] = on_i_do_not_care,
    [msg.ST_REJOIN_GAME_FAIL] = on_i_do_not_care,
    [msg.ST_SET_CAMERA_ROTATE] = on_i_do_not_care,
    [msg.ST_JOB_SKILL_ERROR] = on_i_do_not_care,
    [msg.ST_SYNC_MINISERVER_CITY_POS] = on_i_do_not_care,
    [msg.ST_SHOW_SLOT_MSG_UI] = on_i_do_not_care,
    [msg.ST_SHOW_SLOT_MSG_UI_BY_NAME] = on_i_do_not_care,
    [msg.ST_CHESS_LEFT_SHORT_TIPS] = on_i_do_not_care,
    [msg.ST_KILL_MONSTER_COUNT_TIPS] = on_i_do_not_care,
    [msg.ST_HIDE_LEFT_SHORT_TIPS] = on_i_do_not_care,
    [msg.ST_TRANSFER_MINI_TO_GAME_MSG] = on_i_do_not_care,
    [msg.ST_ACK_JOIN_ACTIVITY_INSTANCE] = on_i_do_not_care,
    [msg.ST_DRAW_DYNAMIC_LAND] = on_i_do_not_care,
    [msg.ST_ACK_QUEST_HEIANZHIMEN_JOIN_PLANE] = on_i_do_not_care,
    [msg.ST_ACK_QUEST_HEIANZHIMEN_RESET_COUNT] = on_i_do_not_care,
    [msg.ST_ACK_QUEST_HEIANZHIMEN_OPEN_RANDOM_CARD] = on_i_do_not_care,
    [msg.ST_REMOVE_MAIL] = on_i_do_not_care,
    [msg.ST_REPLACE_MAIL] = on_i_do_not_care,
    [msg.ST_PICK_FAILED] = on_i_do_not_care,
    [msg.ST_PLAYER_SECTION_GAIN] = on_i_do_not_care,
    [msg.ST_TELL_CLINET_ALREADY_SEND_REQ_PLANE_DROP] = on_i_do_not_care,
    [msg.ST_UNLOCK_TEAM_SKILLS] = on_i_do_not_care,
    [msg.ST_INSTANCE_FINISH_COUNT_DOWN] = on_i_do_not_care,
    [msg.ST_GOBLIN_MARKET_REDRAW_ALL_RES] = on_i_do_not_care,
    [msg.ST_GOBLIN_MARKET_UPDATE_NOTIFY] = on_i_do_not_care,
    [msg.ST_PRAY_UPDATE_NOTIFY] = on_i_do_not_care,
    [msg.ST_PRAY_RESUIT] = on_i_do_not_care,
    [msg.ST_PRAY_EVENT_UPDATE] = on_i_do_not_care,
    [msg.ST_GOBLIN_MARKET_BAG_FULL] = on_i_do_not_care,
    [msg.ST_PRAY_BAG_FULL] = on_i_do_not_care,
    [msg.ST_RECHARGE_RESULT] = on_i_do_not_care,
    [msg.ST_TREASURE_LEFT_TIME] = on_i_do_not_care,
    [msg.ST_BOUGHT_PRIVILEGE_BOX] = on_i_do_not_care,
    [msg.ST_OPEN_STATICS_UI] = on_i_do_not_care,
    [msg.ST_TELL_CLIENT_EXIT_INSTANCE] = on_i_do_not_care,
    [msg.ST_PLAY_ANIM] = on_i_do_not_care,
    [msg.ST_TAMRIEL_JOIN_BATTLE_TIMEOUT] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_QUESTION_STATEUS] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_QUESTION_ANSWER] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_QUESTION_AWARD] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_QUESTION_NEXT] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_QUESTION_RANK_LIST] = on_i_do_not_care,
    [msg.ST_ASK_CLIENT_DO_RECHARGE] = on_i_do_not_care,
    [msg.ST_SHOW_GAIN_DARK_POWER_UI] = on_i_do_not_care,
    [msg.ST_TAMRIEL_SIGN_RESULT] = on_i_do_not_care,
    [msg.ST_TAMRIEL_UPDATE_INFO] = on_i_do_not_care,
    [msg.ST_TAMRIEL_UPDATE_WAITING] = on_i_do_not_care,
    [msg.ST_ACK_QUEST_MAMMONS_TREASURE_INST_COMPLETED] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_PRICE_BOARD_GET_AWARD] = on_i_do_not_care,
    [msg.ST_ACK_EVENT_PRICE_BOARD_STATUS] = on_i_do_not_care,
    [msg.ST_ACK_LUCKY_RATE_DATA] = on_i_do_not_care,
    [msg.ST_TAMRIEL_QUIT_MATCH] = on_i_do_not_care,
    [msg.ST_LOGIN_SESSION_ALIVE_RET] = on_i_do_not_care,
    [msg.ST_LOGIN_CLOSE_SESSION_RET] = on_i_do_not_care,
    [msg.ST_LOGIN_ERR] = on_i_do_not_care,
    [msg.ST_RESELECT_PLAYER_RET] = on_i_do_not_care,
    [msg.ST_RECONNECT] = on_i_do_not_care,
    [msg.ST_KICK_PLAYER] = on_i_do_not_care,
    --[msg.ST_PLAYER_RUNE_ACTIVE] = on_i_do_not_care,
    --[msg.ST_PLAYER_TRANSFORM_CHANGE] = on_i_do_not_care,
    [msg.ST_RECONNECT_FROM_BF] = on_i_do_not_care,
    [msg.ST_ACK_BATTLE_FIELD_DOUBLE_HONOR] = on_i_do_not_care,
    [msg.ST_BAG_FULL_SEND_MAIL] = on_i_do_not_care,
    [msg.ST_SHOW_PRIZE_ITEM] = on_i_do_not_care,
    [msg.ST_ACK_CREATE_NEW_PARCHMENT_QUEST] = on_i_do_not_care,
    [msg.ST_NOTIFY_PARCHMENT_QUEST_FAIL] = on_i_do_not_care,
    [msg.ST_NOTIFY_ENTER_PARCHMENT_INST_SUCC] = on_i_do_not_care,
    [msg.ST_BATFIELD_CRYSTAL_HP_PERCENTAGE] = on_i_do_not_care,
    [msg.ST_GET_COUPON_RES] = on_i_do_not_care,
    [msg.ST_BATTLE_FIELD_READY_GO] = on_i_do_not_care,
    [msg.ST_BATTLE_FIELD_MINI_MAP_SYNC] = on_i_do_not_care,
    [msg.ST_SHOW_NEWBIE_GUIDE_REQ] = on_i_do_not_care,
    [msg.ST_RECV_NEWBIE_GUIDE_MSG] = on_i_do_not_care,
    [msg.ST_SHUTDOWN_MYSTIC_DEALER] = on_i_do_not_care,
    [msg.ST_BUY_GOODS_RES] = on_i_do_not_care,
    [msg.ST_CHAT_ITEM_INFO] = on_i_do_not_care,
    [msg.ST_OBJ_PLAY_SKILL_EFFECT] = on_i_do_not_care,
    [msg.ST_PVP_SOLO_COUNT_DOWN] = on_i_do_not_care,
    [msg.ST_UPDATE_MONSTER_TRANSFORM] = on_i_do_not_care,
    [msg.ST_PLAYER_AVATAR_CHANGE_MAIN] = on_i_do_not_care,
    [msg.ST_PLAYER_AVATAR_CHANGE_LIGHT] = on_i_do_not_care,
    [msg.ST_PLAYER_AVATAR_UPDATE_AVATAR] = on_i_do_not_care,
    [msg.ST_PLAYER_AVATAR_UPDATE_LIGHT] = on_i_do_not_care,
    [msg.ST_PUT_ITEMS_INTO_WAREHOUSE] = on_i_do_not_care,
    [msg.ST_WITHDRAW_WAREHOUSE_ITEM] = on_i_do_not_care,
    [msg.ST_TOWER] = on_i_do_not_care,
    [msg.ST_PALACE] = on_i_do_not_care,
    [msg.ST_PALACE_AVATAR_LIST] = on_i_do_not_care,
    [msg.ST_PALACE_SELECT_YOUR_MODEL] = on_i_do_not_care,
    [msg.ST_SHOW_INST_SECTION_RESULT] = on_i_do_not_care,
    [msg.ST_ONE_DROP_RESULT] = on_i_do_not_care,
    [msg.ST_PLANE_SECTION_DROP] = on_i_do_not_care,
    [msg.ST_SHOW_KILL_BOSS_NOTIFY] = on_i_do_not_care,
    [msg.ST_GET_TOWER_FIRST_PRIZE_RES] = on_i_do_not_care,
    [msg.ST_ARES_JOIN_BF_NOTICE] = on_i_do_not_care,
    [msg.ST_ARES_EVENT_STATE] = on_i_do_not_care,
    [msg.ST_ARES_SIGN_UP] = on_i_do_not_care,
    [msg.ST_ARES_MATCH_BYE] = on_i_do_not_care,
    [msg.ST_ARES_MATCH_READY] = on_i_do_not_care,
    [msg.ST_ARES_MATCH_FINISH] = on_i_do_not_care,
    [msg.ST_ARES_UPDATE_RANK] = on_i_do_not_care,
    [msg.ST_ARES_COUNT_DOWN] = on_i_do_not_care,
    [msg.ST_ARES_MATCH_RESULT] = on_i_do_not_care,
    [msg.ST_ARES_MATCH_SUMMARY] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_HALL_MATCH_RET] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_HALL_OVER] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_HALL_RANK] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_HALL_PLAYER_SCORE] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_HALL_GET_AWARD_RET] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_LADDER_MATCH_STATE] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_LADDER_MATCH_RESULT] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_LADDER_TO_CONFIRM] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_LADDER_SUMMARY] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_LADDER_SELECT_YOUR_MODEL] = on_i_do_not_care,
    [msg.ST_ACK_HONOR_LADDER_MY_RANK] = on_i_do_not_care,
    [msg.ST_SINGLE_GOAL_INFO] = on_i_do_not_care,
    [msg.ST_SHOW_SCENARIO_DIALOG] = on_i_do_not_care,
    [msg.ST_PLAY_HIT_DROP_SFX] = on_i_do_not_care,
    [msg.ST_FINISH_SHOWING_NEWBIE_GUIDE] = on_i_do_not_care,
    [msg.ST_ADD_SHOW_CHESSBOARD_UI] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_REQUEST] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_CANCEL_REQUEST] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_RESPOND] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_ADD_ITEM] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_REMOVE_ITEM] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_MODIFY_GOLD] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_FIRST_CONFIRM] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_START_TRADE] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_CANCEL_TRADE] = on_i_do_not_care,
    [msg.ST_P2P_TRADE_DEAL_RESULT] = on_i_do_not_care,
    [msg.ST_ADD_LEVEL_INFO] = on_i_do_not_care,
    [msg.ST_EXCHANGE_ONSELL_LIST] = on_i_do_not_care,
    [msg.ST_EXCHANGE_CREATE_SUCC] = on_i_do_not_care,
    [msg.ST_EXCHANGE_OPERATE_FAIL] = on_i_do_not_care,
    [msg.ST_EXCHANGE_WITHDREW] = on_i_do_not_care,
    [msg.ST_EXCHANGE_RETRIEVED] = on_i_do_not_care,
    [msg.ST_EXCHANGE_RECEIVED] = on_i_do_not_care,
    [msg.ST_EXCHANGE_TIMEOUT] = on_i_do_not_care,
    [msg.ST_EXCHANGE_SOLD] = on_i_do_not_care,
    [msg.ST_EXCHANGE_BOUGHT] = on_i_do_not_care,
    [msg.ST_PUSH_MYSTIC_GLORY_INFO] = on_i_do_not_care,
    [msg.ST_SHOW_SKILL_USE_TIPS] = on_i_do_not_care,
    [msg.ST_USE_ITEM_NOTIFY] = on_i_do_not_care,
    [msg.ST_USE_RANDOM_ITEM_NOTIFY] = on_i_do_not_care,
    [msg.ST_NOTIFY_CLIENT_MYSTIC] = on_i_do_not_care,
    [msg.ST_RESPONSE_PLAYER_IMAGE] = on_i_do_not_care,
    [msg.ST_ACK_SELECT_SONG] = on_i_do_not_care,
    [msg.ST_SHOW_SELECT_SONG_UI] = on_i_do_not_care,
    [msg.ST_REWARD_ITEMS] = on_i_do_not_care,
    [msg.ST_GET_LEONA_GIFT_RET] = on_i_do_not_care,
    [msg.ST_RECHARGE_AWARD_LOTTERY_RET] = on_i_do_not_care,
    [msg.ST_LEONA_PRESENT] = on_i_do_not_care,
    [msg.ST_REWARD_LOTTERY_ITEMS] = on_i_do_not_care,
    [msg.ST_RETURN_ITEM_MATERIALS] = on_i_do_not_care,
    [msg.ST_RUNNER_START] = on_i_do_not_care,
    [msg.ST_RUNNER_QUIT] = on_i_do_not_care,
    [msg.ST_RUNNER_GIVE_AWARD] = on_i_do_not_care,
    [msg.ST_RUNNER_CHANGE_STATE] = on_i_do_not_care,
    [msg.ST_RUNNER_TRADE_INFO] = on_i_do_not_care,
    [msg.ST_RUNNER_ERROR] = on_i_do_not_care,
    [msg.ST_RUNNER_UPDATE_ITEM] = on_i_do_not_care,
    [msg.ST_QUERY_CLIENT_PROCESS] = on_i_do_not_care,
    [msg.ST_FORCE_CLIENT_EXIT] = on_i_do_not_care,
    [msg.ST_UPDATE_HACKER_BLACKLIST] = on_i_do_not_care,
    [msg.ST_ADD_HOLIDAY_GOT_ITEM] = on_i_do_not_care,
    [msg.ST_ORANGE_EQUIP_PREVIEW] = on_i_do_not_care,
    [msg.ST_ORANGE_EQUIP_ASCEND_PREVIEW] = on_i_do_not_care,
    [msg.ST_GET_RANK_LIST] = on_i_do_not_care,
    [msg.ST_UPDATE_RECHARGE_TOTAL] = on_i_do_not_care,
    [msg.ST_EARL_AWARD_CONSUME_RET] = on_i_do_not_care,
    [msg.ST_SET_TITLE] = on_i_do_not_care,
    [msg.ST_RECHARGE_REWARD_DIAMOND] = on_i_do_not_care,
    [msg.ST_CHAMBERCOMMERCE_RESET] = on_i_do_not_care,
    [msg.ST_WITTYAKNIGHT_RESET] = on_i_do_not_care,
    [msg.ST_ADD_TITLE] = on_i_do_not_care,
    [msg.ST_GET_TRUNCHEON] = on_i_do_not_care,
    [msg.ST_SYNC_TITLE] = on_i_do_not_care,
    [msg.ST_GUILD_ATTR] = on_i_do_not_care,
    [msg.ST_GUILD_MEMBERS] = on_i_do_not_care,
    [msg.ST_GUILD_APPLY_LIST] = on_i_do_not_care,
    [msg.ST_GUILD_MY_INVITATION_LIST] = on_i_do_not_care,
    [msg.ST_GUILD_MY_APPLY_LIST] = on_i_do_not_care,
    [msg.ST_GUILD_OPERATE_RES] = on_i_do_not_care,
    [msg.ST_GUILD_NEW_INVITATION] = on_i_do_not_care,
    [msg.ST_GUILD_NOTIFY_MSG] = on_i_do_not_care,
    [msg.ST_GUILD_MY_APPLY_RES] = on_i_do_not_care,
    [msg.ST_GUILD_KICK_RES] = on_i_do_not_care,
    [msg.ST_GUILD_SET_ROLE_RES] = on_i_do_not_care,
    [msg.ST_GUILD_ADD_MEMBER] = on_i_do_not_care,
    [msg.ST_GUILD_DEL_MEMBER] = on_i_do_not_care,
    [msg.ST_GUILD_UPDATE_MEMBER] = on_i_do_not_care,
    [msg.ST_GUILD_ADD_APPLIER] = on_i_do_not_care,
    [msg.ST_GUILD_DEL_APPLIER] = on_i_do_not_care,
    [msg.ST_GUILD_UPDATE_APPLIER] = on_i_do_not_care,
    [msg.ST_GUILD_SEARCH_RES] = on_i_do_not_care,
    [msg.ST_GUILD_NO_SEARCH_RES] = on_i_do_not_care,
    [msg.ST_GUILD_ADD_UPDATE_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_DEL_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_ADD_UPDATE_INVITE] = on_i_do_not_care,
    [msg.ST_GUILD_DEL_INVITE] = on_i_do_not_care,
    [msg.ST_GUILD_LIST] = on_i_do_not_care,
    [msg.ST_GUILD_INFO] = on_i_do_not_care,
    [msg.ST_PROFESSION_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_ALL_ON_JOB] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_UPDATE_ON_JOB] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_DELETE_ON_JOB] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_ALL_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_UPDATE_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_DELETE_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GLOBAL_DATA] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_ALL_APPLIERS] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_UPDATE_APPLIER] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_ALL_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_UPDATE_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_DELETE_APPLIER] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_GOODS_DELETE_APPLY] = on_i_do_not_care,
    [msg.ST_GUILD_PROF_DROP_RATE] = on_i_do_not_care,
    [msg.ST_GUILD_DONATE_HISTORY] = on_i_do_not_care,
    [msg.ST_GUILD_DONATE_HISTORY_NEW_RECORD] = on_i_do_not_care,
    [msg.ST_GUILD_DONATE_UPDATE_ACTIVATE_ID] = on_i_do_not_care,
    [msg.ST_GUILD_SHOP_PRAY] = on_i_do_not_care,
    [msg.ST_GUILD_CAMPFIRE_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_JOIN_NEW_GUILD] = on_i_do_not_care,
    [msg.ST_GUILD_LEAVE_GUILD] = on_i_do_not_care,
    [msg.ST_GUILD_LOGIN_WITH_GUILD] = on_i_do_not_care,
    [msg.ST_GUILD_CAMPFIRE_START] = on_i_do_not_care,
    [msg.ST_GUILD_CAMPFIRE_STOP] = on_i_do_not_care,
    [msg.ST_GUILD_CAMPFIRE_BENEFIT] = on_i_do_not_care,
    [msg.ST_FRIEND_SET_INFO] = on_i_do_not_care,
    [msg.ST_FRIEND_SEARCH] = on_i_do_not_care,
    [msg.ST_FRIEND_ADD_NORMAL] = on_i_do_not_care,
    [msg.ST_FRIEND_DEL_NORMAL] = on_i_do_not_care,
    [msg.ST_FRIEND_ADD_BLACK] = on_i_do_not_care,
    [msg.ST_FRIEND_DEL_BLACK] = on_i_do_not_care,
    [msg.ST_FRIEND_ACCEPT] = on_i_do_not_care,
    [msg.ST_FRIEND_REJECT] = on_i_do_not_care,
    [msg.ST_FRIEND_BLOCK_PENDING] = on_i_do_not_care,
    [msg.ST_FRIEND_COMMON] = on_i_do_not_care,
    [msg.ST_FRIEND_UPDATE_HISTORY] = on_i_do_not_care,
    [msg.ST_FRIEND_CHAT] = on_i_do_not_care,
    [msg.ST_FRIEND_ADD] = on_i_do_not_care,
    [msg.ST_FRIEND_DEL] = on_i_do_not_care,
    [msg.ST_FRIEND_BASIC_INFO] = on_i_do_not_care,
    [msg.ST_FIRST_RECHARGE_CONSUME_RET] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_SIGN_FAIL] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_MATCH_FAIL] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_OVER] = on_i_do_not_care,
    [msg.ST_TRIGGER_PROGRESS_INFO] = on_i_do_not_care,
    [msg.ST_TRIGGER_PROGRESS_STATE] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_SYNC_MINIMAP] = on_i_do_not_care,
    [msg.ST_TEAM_MATE_SYN_HP] = on_i_do_not_care,
    [msg.ST_TEAM_INVITE_RESULT] = on_i_do_not_care,
    [msg.ST_TEAM_REMOVE_INVITE] = on_i_do_not_care,
    [msg.ST_BATTLE_CHAT_TIP] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_ADD_MATCH_LIST] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_RM_MATCH_LIST] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_DUELING_END] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_SHARK_MINIMAP] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_INVITE_FAIL] = on_i_do_not_care,
    [msg.ST_ADD_UPDATE_TARGETS] = on_i_do_not_care,
    [msg.ST_ADD_TASK_QUESTION] = on_i_do_not_care,
    [msg.ST_ADD_TASK_CREATE_AOI] = on_i_do_not_care,
    [msg.ST_ADD_WEEKLY_SYSTEM_OPEN] = on_i_do_not_care,
    [msg.ST_ADD_TARGET_PROGRESS] = on_i_do_not_care,
    [msg.ST_GUILD_PVP_SEND_STRING_MSG] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_ENTER_RET] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_GOTO_NEXT_ROUND] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_GENERATE_RAND_NUM] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_INTERACTIVE] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_UPDATE_DATA] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_NAV] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_GAIN_ITEMS] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_FINISH_FIGHT] = on_i_do_not_care,
    [msg.ST_PLAYER_HIDE_SKILL_BUTTONS] = on_i_do_not_care,
    [msg.ST_ADD_PUSH_SERVICE] = on_i_do_not_care,
    [msg.ST_WEEKEND_PVP_DEAD_VIEW] = on_i_do_not_care,
    [msg.ST_DROP_DICE_NUMBER] = on_i_do_not_care,
    [msg.ST_DROP_ITEM_DICE] = on_i_do_not_care,
    [msg.ST_DROP_DICE_RESULT] = on_i_do_not_care,
    [msg.ST_WEEKEND_PVP_CANCEL_BATTLE_STATE] = on_i_do_not_care,
    [msg.ST_RT_VOICE] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_INVITE_FRIEND] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_INVITE_JOIN_FAIL] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_INVITE_ADD] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_INVITE_RM] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_REFINE_DATA] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_REFINE] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_REBORN] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_REBORN_CONFIRM] = on_i_do_not_care,
    [msg.ST_STOP_SEND_RT_VOICE] = on_i_do_not_care,
    [msg.ST_EQUIP_EXCHANGE_SHOP_RESET] = on_i_do_not_care,
    [msg.ST_EQUIP_EXCHANGE_RESULT] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_WING] = on_i_do_not_care,
    [msg.ST_ADD_EQUIP_EXCHANGE_SHOP_ITEMS] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_LETTERING] = on_i_do_not_care,
    [msg.ST_TRUNCHEON_LETTERING_CONFIRM] = on_i_do_not_care,
    [msg.ST_DROP_DICE_FAIL] = on_i_do_not_care,
    [msg.ST_WEEKEND_PVP_ENROLL_CONFIRM] = on_i_do_not_care,
    [msg.ST_TEAM_SEND_CONFIRM_RESULT] = on_i_do_not_care,
    [msg.ST_OPEN_MULTI_INST] = on_i_do_not_care,
    [msg.ST_MULTI_INST_UNREVIVE] = on_i_do_not_care,
    [msg.ST_SYN_REVIVE_COUNT] = on_i_do_not_care,
    [msg.ST_COMPETITIVE_HALO] = on_i_do_not_care,
    [msg.ST_ADD_TASK_QUESTION_RES] = on_i_do_not_care,
    [msg.ST_COMPETITVE_HALO_AWARD] = on_i_do_not_care,
    [msg.ST_HONOR_HALL_INVITE_PK] = on_i_do_not_care,
    [msg.ST_HONOR_HALL_INVITE_ACK] = on_i_do_not_care,
    [msg.ST_FORBID_CHAT_NOTIFY] = on_i_do_not_care,
    [msg.ST_SYN_TEAM_MEMBER_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_INST_RESULT] = on_i_do_not_care,
    [msg.ST_PVP_SYNC_MINIMAP_SHARK] = on_i_do_not_care,
    [msg.ST_TEAM_INST_REJOIN] = on_i_do_not_care,
    [msg.ST_MINI_MAP_SINGAL] = on_i_do_not_care,
    [msg.ST_CHANGE_WILD_ROLE] = on_i_do_not_care,
    [msg.ST_WILD_LIST] = on_i_do_not_care,
    [msg.ST_WILD_MSG] = on_i_do_not_care,
    [msg.ST_WILD_PRISON_COUNT_DOWN] = on_i_do_not_care,
    [msg.ST_PLAYER_ATTR_MODIFIED] = on_i_do_not_care,
    [msg.ST_RECHARGE_REMIND_OPEN] = on_i_do_not_care,
    [msg.ST_ADD_TAMRIEL_MATCH_LIST] = on_i_do_not_care,
    [msg.ST_REMOVE_TAMRIEL_MATCH_LIST] = on_i_do_not_care,
    [msg.ST_SYN_ENTER_INST_CONFIRM] = on_i_do_not_care,
    [msg.ST_FETCH_PLAYER_ATTR] = on_i_do_not_care,
    [msg.ST_REPORT_PLAYER_ATTR] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_STATUS] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_PLAYER_POS] = on_i_do_not_care,
    [msg.ST_MULTI_TIPS] = on_i_do_not_care,
    [msg.ST_MILLIONAIRE_COUNTDOWN] = on_i_do_not_care,
    [msg.ST_SET_WATCH_MODE] = on_i_do_not_care,
    [msg.ST_NOTIFY_MULTI_INST] = on_i_do_not_care,
    [msg.ST_POTION_LEFT_COUNT] = on_i_do_not_care,
    [msg.ST_STRONGER_GS_GRADE] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_UPDATE_INFO] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_UPDATE_TEAM_INFO] = on_i_do_not_care,
    [msg.ST_FIVE_PVP_BATTLE_FINISH] = on_i_do_not_care,
    [msg.ST_SET_SKILL_SLOT] = on_i_do_not_care,
    [msg.ST_ACTIVATE_ELEMENT] = on_i_do_not_care,
    [msg.ST_DEACTIVATE_ELEMENT] = on_i_do_not_care,
    [msg.ST_UPGRADE_MASTER_SKILL] = on_i_do_not_care,
    [msg.ST_UPGRADE_PRACTICE_SKILL] = on_i_do_not_care,
    [msg.ST_ELEMENT_ERROR] = on_i_do_not_care,
    [msg.ST_UNLOCK_MASTER_SKILL] = on_i_do_not_care,
    [msg.ST_UNLOCK_PRACTICE_SKILL] = on_i_do_not_care,
    [msg.ST_PVE_ELEMENT] = on_i_do_not_care,
    [msg.ST_PVP_ELEMENT] = on_i_do_not_care,
    [msg.ST_PUTAWAY_ITEM] = on_i_do_not_care,
    [msg.ST_SOLD_OUT_ITEM] = on_i_do_not_care,
    [msg.ST_MODIFY_TRADE_PRICE] = on_i_do_not_care,
    [msg.ST_QUERY_TRADE_ITEM_LIST] = on_i_do_not_care,
    [msg.ST_BUY_TRADE_ITEM] = on_i_do_not_care,
    [msg.ST_ADD_FAVORITE_ITEM] = on_i_do_not_care,
    [msg.ST_RM_FAVORITE_ITEM] = on_i_do_not_care,
    [msg.ST_GET_FAVORITE_ITEM_LIST] = on_i_do_not_care,
    [msg.ST_ITEM_BEEN_SOLD] = on_i_do_not_care,
    [msg.ST_BUY_TRADE_ITEM_FAIL] = on_i_do_not_care,
    [msg.ST_AUCTION_ITEM] = on_i_do_not_care,
    [msg.ST_CANCEL_AUCTION_ITEM] = on_i_do_not_care,
    [msg.ST_QUERY_AUCTION_ITEM] = on_i_do_not_care,
    [msg.ST_BID_AUCTION_ITEM] = on_i_do_not_care,
    [msg.ST_GET_BID_ITEM] = on_i_do_not_care,
    [msg.ST_SYNC_AUCTION_ITEM] = on_i_do_not_care,
    [msg.ST_ITEM_AUCTION_RESULT] = on_i_do_not_care,
    [msg.ST_GET_AUCTION_STATISTIC] = on_i_do_not_care,
    [msg.ST_WIN_AUCTION_ITEM] = on_i_do_not_care,
    [msg.ST_SYNC_CUSTOMER_SERVICE] = on_i_do_not_care,
    [msg.ST_NEW_CUSTOMER_SERVICE_DAILOG] = on_i_do_not_care,
    [msg.ST_EXCHANGE_PURPLE_SOUL] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_START] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_QUIT] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_GIVE_AWARD] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_CHANGE_STATE] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_TRADE_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_ERROR] = on_i_do_not_care,
    [msg.ST_GUILD_MERCHANT_RUNNER_UPDATE_ITEM] = on_i_do_not_care,
    [msg.ST_GUILDPROF_TASK_TARGET] = on_i_do_not_care,
    [msg.ST_TRY_INSCRIPTION_RET] = on_i_do_not_care,
    [msg.ST_GUILDPROF_MERCHANT_ESCORT] = on_i_do_not_care,
    [msg.ST_GUILDPROF_TASK_NOTIFY] = on_i_do_not_care,
    [msg.ST_GUILDPROF_ESCORT_MON] = on_i_do_not_care,
    [msg.ST_INSCRIPTION_BEGIN_ROLL] = on_i_do_not_care,
    [msg.ST_INSCRIPTION_IN_OUT] = on_i_do_not_care,
    [msg.ST_INSCRIPTION_BEGIN_WRITE] = on_i_do_not_care,
    [msg.ST_INSCRIPTION_COME_IN_CD] = on_i_do_not_care,
    [msg.ST_INSCRIPTION_UPDATE_TRIGGER] = on_i_do_not_care,
    [msg.ST_INSCRIPTION_END_WRITING] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_ENROLL_RESULT] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_EVENT_STATE] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_BATTLE_TIPS] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_FORBID_QUIT] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_GUILD_KICK_OUT] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_JOIN_BATTLE_RESULT] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_CURRENT_SCORE] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_BATTLE_RESULT] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_FINISH_BATTLE] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_SET_JOIN_VALUE] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_GUILD_RECORD] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_BATTLE_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_SYNC_FIGHTER_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_SYNC_MASTER_CHANGE] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_SYNC_WATCHER_COUNT] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_CHARACTER_NOTIFY] = on_i_do_not_care,
    [msg.ST_SHOW_CONFIRM_BACK_TO_TOWN] = on_i_do_not_care,
    [msg.ST_ELAM_MILITARY_SUB_RESULT] = on_i_do_not_care,
    [msg.ST_GUILD_WEEKEND_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_JOIN] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_DATA] = on_i_do_not_care,
    [msg.ST_PLAYER_GUILD_PVE_DATA] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_NOTIFY] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_START] = on_i_do_not_care,
    [msg.ST_SEND_INST_KILL_RECORD] = on_i_do_not_care,
    [msg.ST_TRIGGER_PROEVENT_INFO] = on_i_do_not_care,
    [msg.ST_SYN_DOOR_FLAG] = on_i_do_not_care,
    [msg.ST_GUILD_WEEKEND_DEAD_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_WEEKEND_SET_MINIMAP] = on_i_do_not_care,
    [msg.ST_GW_SYN_SEEK_HELP_COUNT] = on_i_do_not_care,
    [msg.ST_GW_SEEK_HELP] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_DONATION] = on_i_do_not_care,
    [msg.ST_GUILD_WEEKEND_STATE] = on_i_do_not_care,
    [msg.ST_GW_ENROLL_RESULT] = on_i_do_not_care,
    [msg.ST_GW_ENTER_RESULT] = on_i_do_not_care,
    [msg.ST_GW_CHECK_ENTER_PUBLIC] = on_i_do_not_care,
    [msg.ST_GW_GAME_OVER_INFO] = on_i_do_not_care,
    [msg.ST_GW_ATTACK_WIN] = on_i_do_not_care,
    [msg.ST_GW_ON_GUILD_DELETE] = on_i_do_not_care,
    [msg.ST_SHOW_CAMP_NOTIFY_UI] = on_i_do_not_care,
    [msg.ST_DROP_DICE_GIVE_UP] = on_i_do_not_care,
    [msg.ST_MINIMAP_EFFECT] = on_i_do_not_care,
    [msg.ST_GUILD_WEEKEND_SYN_MINIMAP] = on_i_do_not_care,
    [msg.ST_RECRUIT_INFO] = on_i_do_not_care,
    [msg.ST_RECRUIT_JOIN_RESULT] = on_i_do_not_care,
    [msg.ST_RECRUIT_QUILT_RESULT] = on_i_do_not_care,
    [msg.ST_RECRUIT_LIST] = on_i_do_not_care,
    [msg.ST_INSTANCE_MINIMAP_INFO] = on_i_do_not_care,
    [msg.ST_INSTANCE_MINIMAP_CLEAR] = on_i_do_not_care,
    [msg.ST_GUILD_WEEKEND_COUNT_DOWN] = on_i_do_not_care,
    [msg.ST_INST_BATTLE_TIP] = on_i_do_not_care,
    [msg.ST_INST_CAMERA_CHANGE] = on_i_do_not_care,
    [msg.ST_BUY_BOUTIQUE_ITEM] = on_i_do_not_care,
    [msg.ST_ASK_PAY_BOUTIQUE_ITEM] = on_i_do_not_care,
    [msg.ST_PAY_BOUTIQUE_ITEM] = on_i_do_not_care,
    [msg.ST_BOUTIQUE_OPEN] = on_i_do_not_care,
    [msg.ST_OPEN_BOUTIQUE] = on_i_do_not_care,
    [msg.ST_QUIT_GUILD] = on_i_do_not_care,
    [msg.ST_GW_BATTLE_INFO] = on_i_do_not_care,
    [msg.ST_MINIMUMDROP_INFO] = on_i_do_not_care,
    [msg.ST_RECRUIT_START_RESULT] = on_i_do_not_care,
    [msg.ST_RECRUIT_TEAM_RESULT] = on_i_do_not_care,
    [msg.ST_RECRUIT_MAKE_TEAM] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_GOT_FIGHT_SCORE] = on_i_do_not_care,
    [msg.ST_RECRUIT_EVENT_LIST] = on_i_do_not_care,
    [msg.ST_TEAM_START_RECRUIT_RESULT] = on_i_do_not_care,
    [msg.ST_REPLAY_MSG] = on_i_do_not_care,
    [msg.ST_REPLAY_DONE] = on_i_do_not_care,
    [msg.ST_REPLAY_ENABLER] = on_i_do_not_care,
    [msg.ST_REPLAY_START_RECORD] = on_i_do_not_care,
    [msg.ST_REPLAY_STOP_RECORD] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_CREATE_TEAM] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_DELETE_TEAM] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_JOIN_TEAM] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_QUIT_TEAM] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_KICK_TEAM] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_ENROLL_TEAM_DATA] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_ACT_DATA] = on_i_do_not_care,
    [msg.ST_SYN_MONSTER_TIGE_MODE] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_INST_DATA] = on_i_do_not_care,
    [msg.ST_SYN_MONSTER_SHARE_HP] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_REPARE_RET] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_PICK_REPARE] = on_i_do_not_care,
    [msg.ST_MONSTER_HP_ACTION] = on_i_do_not_care,
    [msg.ST_SYN_MONSTER_SHARE_TIGE] = on_i_do_not_care,
    [msg.ST_PLAY_MOVIE] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_WAVE_DATA] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_REWARD_DATA] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_NEXT_PRIZE_LEVEL] = on_i_do_not_care,
    [msg.ST_GOD_DIVINATION_INFO] = on_i_do_not_care,
    [msg.ST_GOD_DIVINATION_RESULT] = on_i_do_not_care,
    [msg.ST_GOD_DIVINATION_DRAW_RESULT] = on_i_do_not_care,
    [msg.ST_GOD_DIVINATION_SHORT_INFO] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_REWARD_GOT_ITEM] = on_i_do_not_care,
    [msg.ST_STOP_CINEMA] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_PRIZE_LOTTERY_START] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_PRIZE_LOTTERY_END] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_REWARD_ATTEND] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_REWARD_END] = on_i_do_not_care,
    [msg.ST_SDKLOG_PLAYER_UPGRADE] = on_i_do_not_care,
    [msg.ST_SDKLOG_CREATE_ORDER] = on_i_do_not_care,
    [msg.ST_SDKLOG_COMPLETE_ORDER] = on_i_do_not_care,
    [msg.ST_SHOW_PLAYER_GPS_INFO_NEARBY] = on_i_do_not_care,
    [msg.ST_SYNC_ADD_PLAYER_RED] = on_i_do_not_care,
    [msg.ST_SYNC_DEL_PLAYER_RED] = on_i_do_not_care,
    [msg.ST_SYNC_ADD_GS_RED] = on_i_do_not_care,
    [msg.ST_SYNC_DEL_GS_RED] = on_i_do_not_care,
    [msg.ST_PICK_RED_RES] = on_i_do_not_care,
    [msg.ST_RED_START] = on_i_do_not_care,
    [msg.ST_PLAYER_RED_DATA] = on_i_do_not_care,
    [msg.ST_RED_REWARD_ITEMS] = on_i_do_not_care,
    [msg.ST_RECRUIT_ANNOUNCEMENT] = on_i_do_not_care,
    [msg.ST_PUSH_TOPIC_SUBSCRIBE] = on_i_do_not_care,
    [msg.ST_FACEBOOK_SHARE_SUCCESS] = on_i_do_not_care,
    [msg.ST_GUILD_PVE_ENTER_SUB_INST] = on_i_do_not_care,
    [msg.ST_SYNC_SHARE] = on_i_do_not_care,
    [msg.ST_GET_SHARE_REWARD] = on_i_do_not_care,
    [msg.ST_SYNC_TRADE_ITEM] = on_i_do_not_care,
    [msg.ST_SYNC_TRADE_RECORD] = on_i_do_not_care,
    [msg.ST_GUILD_DAILY_PVP_DUEL_COUNTDOWN] = on_i_do_not_care,
    [msg.ST_GET_LEVEL_REWARD] = on_i_do_not_care,
    [msg.ST_PET_UNLOCK] = on_i_do_not_care,
    [msg.ST_PET_ACTIVE] = on_i_do_not_care,
    [msg.ST_PET_RENAME] = on_i_do_not_care,
    [msg.ST_HOLIDAY_SIGNIN_AWARD_INFO] = on_i_do_not_care,
    [msg.ST_HOLIDAY_SIGNIN_AWARD_GET_RES] = on_i_do_not_care,
    [msg.ST_ACK_TREASURE_HUNT_SHOP_DATA] = on_i_do_not_care,
    [msg.ST_ACK_TREASURE_HUNT_BUY_MAP] = on_i_do_not_care,
    [msg.ST_SYNC_RECENT_CHAT] = on_i_do_not_care,
    [msg.ST_ACK_TREASURE_HUNT_USE_MAP] = on_i_do_not_care,
    [msg.ST_ACK_TREASURE_HUNT_INFO] = on_i_do_not_care,
    [msg.ST_ACK_TREASURE_HUNT_ABANDON_MAP] = on_i_do_not_care,
    [msg.ST_OPEN_RECENT_CHAT] = on_i_do_not_care,
    [msg.ST_TREASURE_HUNT_ASK_CONINUE] = on_i_do_not_care,
    [msg.ST_TREASURE_HUNT_PERSON_EVENT] = on_i_do_not_care,
    [msg.ST_TREASURE_HUNT_GIVE_AWARD] = on_i_do_not_care,
    [msg.ST_AFTER_CLIENT_ADD_ACTIVE_PLAYER_ON_BF] = on_i_do_not_care,
    [msg.ST_TREASURE_HUNT_MY_MAP_NUM] = on_i_do_not_care,
    [msg.ST_TEST_READ_INT] = on_i_do_not_care,
    [msg.ST_ORANGE_EQUIP_AWAKE_PREVIEW] = on_i_do_not_care,
    [msg.ST_MONTHLY_SIGNIN_AWARD_INFO] = on_i_do_not_care,
    [msg.ST_MONTHLY_SIGNIN_AWARD_GET_RES] = on_i_do_not_care,
    [msg.ST_BROADCAST_ACTIVE_PET_CHANGE] = on_i_do_not_care,
    [msg.ST_MALL_TIME] = on_i_do_not_care,
    [msg.ST_MALL_BUY_RECORD] = on_i_do_not_care,
    [msg.ST_MALL_BUY_RESULT] = on_i_do_not_care,
    [msg.ST_DAILY_ONLINE_AWARD_INFO] = on_i_do_not_care,
    [msg.ST_DAILY_ONLINE_AWARD_GET_RES] = on_i_do_not_care,
    [msg.ST_CYCLED_RECHARGE_AWARD_INFO] = on_i_do_not_care,
    [msg.ST_CYCLED_RECHARGE_AWARD_GET_RES] = on_i_do_not_care,
    [msg.ST_GROWTH_FUND_INFO] = on_i_do_not_care,
    [msg.ST_GROWTH_FUND_GET_RES] = on_i_do_not_care,
    [msg.ST_MALL_SYN_RECORD] = on_i_do_not_care,
    [msg.ST_MALL_RANDOM_RECORD] = on_i_do_not_care,
    [msg.ST_GET_LOGIN_AWARD] = on_i_do_not_care,
    [msg.ST_GET_RECHARGE_AWARD] = on_i_do_not_care,
    [msg.ST_BUY_HALF_PRICE_ITEM] = on_i_do_not_care,
    [msg.ST_REFRESH_LOGIN_ACTIVITY_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_OPERATION_RESULT] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_SEASON_STATE] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_TEAM_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DEL_PLAYER_TIME_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DEL_TEAM_TIME_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_TEAM_LIST] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_NEW_PLAYER_TIME_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_TEAM_APPLY_LIST] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_UPDATE_NOT_FULL_TEAM] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DEL_NOT_FULL_TEAM] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_NOTIFY_REFRESH_TEAM_LIST] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_NEW_TEAM_TIME_INFO] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_CLEAR_SEASON_DATA] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_KICK_OUT_NOTIFY] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_QUIT_TEAM] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_BATTLE_RESULT] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_ADD_MEMBER] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_UPDATE_MEMBER] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DEL_MEMBER] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_CHANGE_CAPTAIN] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_UPDATE_BATTLE_STATE] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_UPDATE_PERISH_STATE] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_SHOW_MATCH_CONFIRM] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_SET_MEMBER_CONFIRM_STATE] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_MESSAGE_TIPS] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_CONVENE_BATTLE] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_SELECT_MODEL] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DEAD_VIEW] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_UPDATE_TEAM_TEXT] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_SEARCH_TEAM_RESULT] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_RECRUIT_MSG] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_ADD_JOIN_MATCH_MEMBER] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DEL_JOIN_MATCH_MEMBER] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_TEAM_RANK] = on_i_do_not_care,
    [msg.ST_TEAM_LADDER_DAILY_BATTLE_CNT] = on_i_do_not_care,
    [msg.ST_ADD_CELEBRATION_INFO] = on_i_do_not_care,
    [msg.ST_ADD_GET_AWARD_RES] = on_i_do_not_care,
    [msg.ST_ADD_LOTTERY_RES] = on_i_do_not_care,
    [msg.ST_ADD_INVEST_RES] = on_i_do_not_care,
    [msg.ST_ADD_CELE_LOTTERY_INFO] = on_i_do_not_care,
    [msg.ST_ADD_LOTTERY_GOODS_LIBRARY] = on_i_do_not_care,
    [msg.ST_NOTICE_REBOOT] = on_i_do_not_care,
    [msg.ST_MISC_MSG_RESULT] = on_i_do_not_care,
    [msg.ST_RECHARGE_MSG_RESULT] = on_i_do_not_care,
    [msg.ST_SANDING] = on_i_do_not_care,
    [msg.ST_OFFICER_BOX_INFO] = on_i_do_not_care,
    [msg.ST_OPEN_SURVEY] = on_i_do_not_care,
    [msg.ST_FINISH_SURVEY] = on_i_do_not_care,
    [msg.ST_NOTIFY_WEEKLY_LOTTERY_STATUS] = on_i_do_not_care,
    [msg.ST_SYN_MALL_VERSION] = on_i_do_not_care,
    [msg.ST_SYN_MALL_PRODUCT] = on_i_do_not_care,
    [msg.ST_QUICK_MATCH_INST] = on_i_do_not_care,
    [msg.ST_CONFIRM_QUICK_MATCH] = on_i_do_not_care,
    [msg.ST_CANCEL_QUICK_MATCH] = on_i_do_not_care,
    [msg.ST_SKILL_SECRET] = on_skill_secret,
    [msg.ST_TRUNCHEON_REBORN_EXCHANGE] = on_i_do_not_care,
    [msg.ST_MINI_RECHARGE_AWARD_RESPONSE] = on_i_do_not_care,
    [msg.ST_TOWER_RANK_LIST] = on_i_do_not_care,
    [msg.ST_PALACE_RANK_LIST] = on_i_do_not_care,
    [msg.ST_MYSTIC_DEALER_REFRESHED_NOTIFY] = on_i_do_not_care,
    [msg.ST_TOWER_RANK_PROMOTED] = on_i_do_not_care,
    [msg.ST_PALACE_RANK_PROMOTED] = on_i_do_not_care,
    [msg.ST_CLEAR_SYSTEM_UNLOCK_TIPS] = on_i_do_not_care,
    [msg.ST_NOTIFY_CLIENT_LOTTERY_REFRESH] = on_i_do_not_care,
    [msg.ST_BATTLE_PRESTIGE] = on_i_do_not_care,
    [msg.ST_BROADCAST_UPDATE_HALL_DATA] = on_i_do_not_care,
    [msg.ST_ADD_UPDATE_HALL_DATA] = on_i_do_not_care,
    [msg.ST_FIFTEENTH_TARGET_ACHIEVE] = on_i_do_not_care,
    [msg.ST_GET_TASK_AWARD] = on_i_do_not_care,
    [msg.ST_LOGIN_TASK_FINISH] = on_i_do_not_care,
    [msg.ST_RESET_WORLDMAP_INST_ENTER_COUNT] =  on_i_do_not_care,
    [msg.ST_FIRST_RECHARGE_INFO] =  on_i_do_not_care,
    [msg.ST_REFRESH_PLAYER_LIKE] =  on_i_do_not_care,
    [msg.ST_CARNIVAL_UPDATE_AWARD] =  on_i_do_not_care,
    [msg.ST_CARNIVAL_UPDATE_APPLY] =  on_i_do_not_care,
    [msg.ST_CARNIVAL_UPDATE_ONLINE] =  on_i_do_not_care,

}
-- s/^\(ST\S*\) .*$/    [msg.\1] = on_i_do_not_care,
function msg_id_2_name(msg_id)
    for k,v in pairs(msg) do
        if v == msg_id then
            return k
        end
    end
    return "msg_not_found"
end

function message_handler( _ar, _msg, _size, _packet_type, _dpid )

    c_trace("Recv Msg:%s  Id:0x%08x",  msg_id_2_name(_packet_type), _packet_type)

    if( func_table[_packet_type] ) then 
        (func_table[_packet_type])( _ar, _dpid, _size )
    else 
        --c_debug()
        c_err( sformat( "[LUA_MSG_HANDLE](robotclient) unknown packet 0x%08x %08x", _packet_type, _dpid) )
    end  
end

function send_disconnect( _dpid )
    local ar = g_ar 
    ar:flush_before_send( msg.PT_DISCONNECT )
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_quit_login_queue( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.PT_QUIT_LOGIN_QUEUE )
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_certify_to_game( _dpid, _account )
    local ar = g_ar     
    ar:flush_before_send( msg.PT_CERTIFY )
	ar:write_double( _account.account_id_ )
    ar:write_int( _account.login_token_ )
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_get_random_name( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.PT_GET_RANDOM_NAME )
    ar:write_ubyte( 1 )  -- gender: male
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_robot_add_item(_dpid)
    local ar = g_ar
    ar:flush_before_send( msg.PT_ROBOT_ADD_ITEM )
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_auto_create_player( _dpid, _account, _player_name )
    local job_id = mrandom(4) 
    if not _player_name or _player_name == "" then
        _player_name = "robotclient_" .. _account.account_name_
    end
    -- 注释掉的代码用于测试名字不合法的情况，勿删
    --_player_name = "1234565"  -- 全是数字
    --_player_name = "ha ha"  -- 含空格
    --_player_name = "ha\tha"  -- 含制表符
    --_player_name = "ha\rha"  -- 含回车符
    --_player_name = "ha\nha"  -- 含换行符
    local slot = 0
    local ar = g_ar     
    ar:flush_before_send( msg.PT_CREATE_PLAYER )
    ar:write_double( _account.account_id_ )
    ar:write_ulong( job_id )
    ar:write_string( _player_name )
    ar:write_ulong( slot )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robotclient](send_auto_create_player) creating player (player_name: '%s', job_id: %d, slot: %d) for account '%s' (account_id: %d)"
         , _player_name, job_id, slot, _account.account_name_, _account.account_id_
         )
end

function send_delete_player( _dpid, _account_id, _player_id )
    local ar = g_ar     
    ar:flush_before_send( msg.PT_DELETE_PLAYER )
    ar:write_ulong( _account_id )
    ar:write_ulong( _player_id )
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_join( _dpid, _player_id, _rand_key )
    local dpid_info = g_dpid_mng[_dpid]
    dpid_info.player_id_ = _player_id
    local ar = g_ar     
    ar:flush_before_send( msg.PT_JOIN )
    ar:write_ulong( _player_id )
    ar:write_ulong( _rand_key )
    ar:write_double( 0 )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )

    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robotclient](send_join) dpid: 0x%X, player_id: %d", _dpid, _player_id )
end

function send_rejoin( _dpid, _dpid_info )
    local player_id = _dpid_info.player_id_
    local rejoin_certify_code = _dpid_info.rejoin_certify_code_
    local ar = g_ar
    ar:flush_before_send( msg.PT_REJOIN )
    ar:write_ulong( player_id )
    ar:write_int( rejoin_certify_code )
    
    ar:write_double( 0 )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:write_string( "robot" )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robotclient](send_rejoin) dpid: 0x%X, player_id: %d, rejoin_certify_code: %d", _dpid, player_id, rejoin_certify_code )
end

function send_hero_join_succ( _dpid )
    local ar = g_ar     
    ar:flush_before_send( msg.PT_AFTER_ADD_HERO )
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_ping( _dpid )
    local ar = g_ar
    ar:flush_before_send( msg.PT_PING )
    ar:write_uint( 0 )
    ar:write_uint( 0 )
    ar:send_one_ar( g_robotclient, _dpid )
    return 1
end

robot_type_func_table =
{
    [ROBOT_TYPE_DO_NOTHING]    = robot_nothing.after_hero_added,
    [ROBOT_TYPE_RANDOM_MOVE]   = robot_move.after_hero_added,
    [ROBOT_TYPE_RANDOM_SKILL]  = robot_skill.after_hero_added,
    [ROBOT_TYPE_SHOP]          = robot_shop.after_hero_added,
    [ROBOT_TYPE_CHAT]          = robot_chat.after_hero_added,
    [ROBOT_TYPE_RECHARGE]      = robot_recharge.after_hero_added,
    [ROBOT_TYPE_INSTANCE]      = robot_instance.after_hero_added,
    [ROBOT_TYPE_TEAM]          = robot_team.after_hero_added, 
    [ROBOT_TYPE_MATCH]          = robot_tamriel_match.after_hero_added, 
    [ROBOT_TYPE_TRADE]          = robot_trade.after_hero_added, 
}

miserver_func_table =
{
}



