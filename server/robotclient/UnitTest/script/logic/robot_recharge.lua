module( "robot_recharge", package.seeall )

g_order_id_index = g_order_id_index or 1

local function get_order_id()
    g_order_id_index = g_order_id_index + 1
    return g_order_id_index
end

local function send_recharge_new_msg( _dpid, _ctrl_id )
    local product_id = math.random( 2, 7 )
    local order_id = get_order_id()

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_RECHARGE_NEW_REQUEST )
    ar:write_int( product_id )
    ar:write_double( order_id )
    ar:write_string( "test" )
    ar:send_one_ar( g_robotclient, _dpid )

    return product_id, order_id
end

local function send_recharge_cancel_msg( _dpid, _ctrl_id, _order_id )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_RECHARGE_CANCEL_REQUEST )
    ar:write_double( _order_id )
    ar:send_one_ar( g_robotclient, _dpid )
end

local function send_recharge_complete_msg( _dpid, _ctrl_id, _order_id )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_RECHARGE_COMPLETE_REQUEST )
    ar:write_double( _order_id )
    ar:send_one_ar( g_robotclient, _dpid )
end

local function send_recharge_continue_msg( _dpid, _ctrl_id, _order_id, _product_id )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_RECHARGE_CONTINUE_REQUEST )
    ar:write_int( _product_id )
    ar:write_double( _order_id )
    ar:write_string( "test" )
    ar:send_one_ar( g_robotclient, _dpid )
end

function timer_send_recharge_request( _dpid, _ctrl_id )
    local hero = ctrl_mng.get_ctrl( _dpid, _ctrl_id )
    if not hero then
        return 0
    end
    local product_id, order_id = send_recharge_new_msg( _dpid, _ctrl_id )
    local index = order_id % 3
    if index == 0 then
        send_recharge_cancel_msg( _dpid, _ctrl_id, order_id )
    elseif index == 1 then
        send_recharge_complete_msg( _dpid, _ctrl_id, order_id )
    elseif index == 2 then
        send_recharge_continue_msg( _dpid, _ctrl_id, order_id, product_id )
    end
    return 1
end

function after_hero_added( _dpid, _hero )
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    local ctrl_id = _hero.ctrl_id_
    if dpid_info.dpid_type_ == robotclient.DPID_TYPE_GAME then
        g_timermng:c_add_timer_second( 1, timers.REQUEST_RECHARGE, _dpid, ctrl_id, 1 )
        c_log( "[robot_recharge] after_hero_added, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    end
end



