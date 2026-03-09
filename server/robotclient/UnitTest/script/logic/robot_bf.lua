module( "robot_bf", package.seeall )

local g_ar = g_ar or LAr:c_new()

function after_hero_added( _dpid, _hero )
    local ctrl_id = _hero.ctrl_id_
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    if dpid_info.dpid_type_ == robotclient.DPID_TYPE_GAME then
        g_timermng:c_add_timer_second( 0, timers.JOIN_FIVE_PVP, _dpid, 0, 10 )
        c_log( "[robot_bf](after_hero_added) after_hero_added, join five pvp, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    end
end

function send_all_to_five_pvp()
    for dpid, info in pairs(robotclient.g_dpid_mng) do
        send_join_five_pvp(dpid)
    end
end

-- step 1
function send_join_five_pvp( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then
        c_err( "[robot_bf](send_join_bf) hero not found, dpid: 0x%X", _dpid )
        return
    end

    robot_gm.energy( hero, 100 )
    
    local ar = g_ar
    ar:flush_before_send( msg.PT_JOIN_FIVE_PVP_MATCH )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_bf](send_join_bf) dpid: 0x%X ", _dpid)
end

function print_all_player()
    for dpid, info in pairs(robotclient.g_dpid_mng) do
        print(dpid) 
    end
end

-- return 1
function on_five_pvp_match_result(_ar, _dpid, _size) 
   local succ = _ar:read_byte() 
   if succ== 1 then
       local player_num = _ar:read_int()
       for i=1,player_num do 
           _ar:read_string()
           _ar:read_int()
           _ar:read_uint()

           _ar:read_uint()
           _ar:read_uint()
       end
    else
        c_err("[robot_bf] enter fail %d", _dpid) 
    end

    c_log("[robot_bf](on five pvp match result) %d", _dpid)
end

-- step 2
function send_scene_load_success(_dpid) 
    local hero = robotclient.get_hero( _dpid )
    if not hero then
        c_err( "[robot_bf](send_instance_preload_finish) hero not found, dpid: 0x%X", _dpid )
        return
    end

    local ar = g_ar
    ar:flush_before_send( msg.PT_LOADING_SUCCESS)
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_bf](send_loading success) dpid: 0x%X", _dpid)
end

-- end 
function on_winer_count_down(_ar, _dpid, _size)
    local bt = _ar:read_byte() 
    if bt == 1 then
        _ar:read_float()
    end
    c_log("[robot_bf](on_winner count down)")
end

function on_ack_five_pvp_status(_ar, _dpid, _size)
    local rejoin = _ar:read_int()
end
