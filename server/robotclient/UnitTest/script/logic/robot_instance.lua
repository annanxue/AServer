module( "robot_instance", package.seeall )

function after_hero_added( _dpid, _hero )
    local ctrl_id = _hero.ctrl_id_
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    if dpid_info.dpid_type_ == robotclient.DPID_TYPE_GAME then
        g_timermng:c_add_timer_second( 5, timers.JOIN_INSTANCE, _dpid, 0, 60   )  
        --g_timermng:c_add_timer_second( 60 * 10, timers.QUIT_INSTANCE, _dpid, 0, 60 * 10    )  
        c_log( "[robot_instance](after_hero_added) after_hero_added, join instance, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    end
end

function send_join_tamriel( _dpid ) 
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_JOIN_BATTLEFIELD_MATCH );
    ar:write_int( 2 ) 
    ar:send_one_ar( g_robotclient, _dpid ) 
end


-- step 1
function send_join_instance( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then
        c_err( "[robot_instance](send_join_instance) hero not found, dpid: 0x%X", _dpid )
        return
    end

     
    if hero.instance_id_ then
        --send_confirm_quit_instance( _dpid ) 
        --c_err( "[robot_instance](send_join_instance) already in instance, dpid: 0x%X, instance_id: %d", _dpid, hero.instance_id_ )
        return
    end
    local dpid_info = robotclient.g_dpid_mng[_dpid]

    robot_gm.energy( hero, 100 )
    
   -- local instance_id = 123101
    local instance_id  = 400000
    --local instance_id  = 111101
    hero.instance_id_ = instance_id

    local skill_secret =  ( dpid_info.skill_secret_ + dpid_info.player_id_ ) % 95273 
    local enter_type   = instance_t.PLANE_ENTER_TYPE_PRICE_BOARD
    local extra_param = 1

    local ar = robotclient.g_ar
    local client_time = os.time()
    ar:flush_before_send( msg.PT_REQ_JOIN_ACTIVITY_INSTANCE )
    local xor_instance_id = c_xor(instance_id , skill_secret )
    ar:write_int( xor_instance_id )
    local xor_enter_type = c_xor(enter_type , skill_secret )
    ar:write_int( xor_enter_type )
    local xor_extra = c_xor(extra_param , skill_secret)
    ar:write_int(  xor_extra )
    local xor_client_time = c_xor(client_time ,  skill_secret)
    ar:write_int( xor_client_time )
    -- 计算密钥
    local part1 = ( instance_id + enter_type + extra_param + dpid_info.player_id_  ) % 10000
    local part2 = client_time % 1000 
    local part3 = skill_secret  % 1000
    local client_sign =  part1 + part2 * part3  
    local xor_sign = c_xor( client_sign , skill_secret )
    ar:write_int( xor_sign  )

    ar:write_int( skill_secret) 

    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_instance](send_join_instance) dpid: 0x%X, instance_id: %d", _dpid, instance_id )
end

-- step 6
function send_instance_preload_finish( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then
        c_err( "[robot_instance](send_instance_preload_finish) hero not found, dpid: 0x%X", _dpid )
        return
    end
    
    local instance_id = hero.instance_id_
    if not instance_id then
        c_err( "[robot_instance](send_instance_preload_finish) instance_id not found, dpid: 0x%X, player_id: %d", _dpid, hero.player_id_ )
        return
    end

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_INSTANCE_PRELOAD_FINISH )
    ar:write_int( instance_id )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_instance](send_instance_preload_finish) dpid: 0x%X, instance_id: %d", _dpid, instance_id )
end

-- step 7 ( see on_replace)
-- step 8
function send_loading_succ( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then
        c_err( "[robot_instance](send_loading_succ) hero not found, dpid: 0x%X", _dpid )
        return
    end

    local instance_id = hero.instance_id_
    if not instance_id then
        c_err( "[robot_instance](send_loading_succ) instance_id not found, dpid: 0x%X, player_id: %d", _dpid, hero.player_id_ )
        return
    end

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_LOADING_SUCCESS )
    ar:write_int( 0 ) -- _client_inst
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_instance](send_loading_succ) dpid: 0x%X, instance_id: %d", _dpid, instance_id )

    -- 通过GM指令完成副本
    robot_gm.finish_inst( hero )
end

function send_confirm_quit_instance( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then
        c_err( "[robot_instance](send_confirm_quit_instance) hero not found, dpid: 0x%X", _dpid )
        return
    end
    
    local instance_id = hero.instance_id_
    if not instance_id then
        c_err( "[robot_instance](send_confirm_quit_instance) instance_id not found, dpid: 0x%X, player_id: %d", _dpid, hero.player_id_ )
        return
    end

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_CONFIRM_QUIT_INSTANCE )
    ar:write_int( instance_id )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_instance](send_confirm_quit_instance) dpid: 0x%X, instance_id: %d", _dpid, instance_id )
end

-- step 2
function on_join_instance_succ( _ar, _dpid, _size )
    local instance_id = _ar:read_int()
    c_log( "[robot_instance](on_join_instance_succ) dpid: 0x%X, instance_id: %d", _dpid, instance_id )
end

function on_join_instance_fail( _ar, _dpid, _size )
    local err_code = _ar:read_byte()
    c_err( "[robot_instance](on_join_instance_fail) dpid: 0x%X, err_code: %d", _dpid, err_code )

    local hero = robotclient.get_hero( _dpid )
    if hero then
        hero.instance_id_ = nil
    end
end

function on_quit_instance_succ( _ar, _dpid, _size )
    local instance_id = _ar:read_int()
    local is_gamesvr = _ar:read_byte() > 0

    local hero = robotclient.get_hero( _dpid )
    if hero then
        hero.instance_id_ = nil
    end
    c_err( "[robot_instance](on_quit_instance_succ) dpid: 0x%X, instance_id: %d", _dpid, instance_id )
end

-- step 3
function on_preload_drop_item_model( _ar, _dpid, _size )
    local num = _ar:read_byte()
    for i = 1, num do
        local model_id = _ar:read_int()
    end
end

-- step 4
function on_miniserver_create_instance( _ar, _dpid, _size )
    _ar:read_buffer()
    -- step 5
    -- on_end_instance_spawning
    -- 单机版
    send_instance_preload_finish( _dpid )
end

function on_start_instance_spawning( _ar, _dpid, _size )
end

function on_end_instance_spawning( _ar, _dpid, _size )
    -- 网络版
    send_instance_preload_finish( _dpid )
end

function on_start_ultimate_skill_cd( _ar, _dpid, _size )
end

function on_play_music( _ar, _dpid, _size )
    local music_id = _ar:read_int()
end

function on_plane_drop( _ar, _dpid, _size )
    local act_inst_id = _ar:read_ulong()
    local gold = _ar:read_ulong()
    local exp = _ar:read_ulong()
    local use_time = _ar:read_ulong()
    local item_count = _ar:read_ulong()

    for i = 1, item_count, 1 do
        local item = item_t.unserialize_item_for_client( _ar )
    end

    local enter_type = _ar:read_ubyte()
    local is_level_up = _ar:read_ubyte()
    local new_level = _ar:read_int()
    local is_send_mail = _ar:read_ubyte()
    local progress = _ar:read_int()
    local diamond_card_rate = _ar:read_float()
    local holiday_double_rate = _ar:read_float()
    local instance_base_exp = _ar:read_float()
    local activity_exp = _ar:read_float()
    local hazm_power_rate = _ar:read_float()
    local witt_rate = _ar:read_float()
    local chamber_add_rate = _ar:read_float()
end

function on_quit_instance( _ar, _dpid, _size )
    local world_id = _ar:read_int()
    local inst_id = _ar:read_int()
    local errcode_id = _ar:read_int()
    local dely = _ar:read_int()

    c_log( "[robot_instance](on_quit_instance) dpid: 0x%X, world_id: %d, inst_id: %d, errcode_id: %d, dely: %d", _dpid, world_id, inst_id, errcode_id, dely )
    send_confirm_quit_instance( _dpid )
end

