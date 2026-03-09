module( "timers", package.seeall )

local sformat = string.format

g_func_map = g_func_map or {}

---------------------------------------------
--              timer id 
---------------------------------------------

AUTO_DISCONNECT_TEST_TIMER = 1
AUTO_QUIT_QUEUE_TEST_TIMER = 2
RANDOM_MOVE_TO = 3
RANDOM_DO_SKILL = 4
CREATE_SHOP = 5
CREATE_SHOP_GOLD_NOT_ENOUGH = 6
GET_SHOP_LIST = 11
GET_MY_SHOP = 12
GET_SHOP_BID_GOLD_INFO = 13
PING = 14
CHAT = 15
REQUEST_RECHARGE = 16
JOIN_INSTANCE = 17
UPDATE_TEAM_TIMER = 18
PUTAWAY_ITEM = 19
ROBOT_TAMERIEL_MATCH = 20
QUIT_INSTANCE = 21 

---------------------------------------------
--              timer func
---------------------------------------------

g_func_map[ AUTO_DISCONNECT_TEST_TIMER ] = function( _p1, _p2 )
    robotclient.send_disconnect( _p1 )
    c_log("[ROBOT](auto_disconnect_timer_callback)")
    return 0
end

g_func_map[ AUTO_QUIT_QUEUE_TEST_TIMER ] = function( _p1, _p2 )
    robotclient.send_quit_login_queue( _p1 )
    robotclient.g_quit_queue_timer_map[ _p1 ] = nil
    c_log("[ROBOT](auto_quit_queue_timer_callback)")
    return 0
end

g_func_map[ RANDOM_MOVE_TO ] = function( _dpid, _ctrl_id )
    return robot_move.timer_send_random_move_to( _dpid, _ctrl_id )
end

g_func_map[ RANDOM_DO_SKILL ] = function( _dpid, _ctrl_id )
    return robot_skill.timer_send_random_do_skill( _dpid, _ctrl_id )
end

g_func_map[ CREATE_SHOP ] = function( _dpid, _ctrl_id )
    return robot_shop.timer_send_create_shop( _dpid, _ctrl_id )
end

g_func_map[ CREATE_SHOP_GOLD_NOT_ENOUGH ] = function( _dpid, _ctrl_id )
    return robot_shop.timer_send_create_shop_gold_not_enough( _dpid, _ctrl_id )
end

g_func_map[ GET_SHOP_LIST ] = function( _dpid, _ctrl_id )
    return robot_shop.timer_get_shop_list( _dpid, _ctrl_id )
end

g_func_map[ GET_MY_SHOP ] = function( _dpid, _ctrl_id )
    return robot_shop.timer_get_my_shop( _dpid, _ctrl_id )
end

g_func_map[ GET_SHOP_BID_GOLD_INFO ] = function( _dpid, _ctrl_id )
    return robot_shop.timer_get_shop_bid_gold_info( _dpid, _ctrl_id )
end

g_func_map[ PING ] = function( _dpid, _p2 )
    return robotclient.send_ping( _dpid )
end

g_func_map[ CHAT ] = function( _dpid, _ctrl_id )
    return robot_chat.timer_send_chat_msg( _dpid, _ctrl_id )
end

g_func_map[ REQUEST_RECHARGE ] = function( _dpid, _ctrl_id )
    return robot_recharge.timer_send_recharge_request( _dpid, _ctrl_id )
end

g_func_map[ JOIN_INSTANCE ] = function( _dpid )
    robot_instance.send_join_instance( _dpid )
    return 1
end

g_func_map[ UPDATE_TEAM_TIMER ] = function( _dpid ) 
    robot_team.on_timer( _dpid ) 
end

g_func_map[PUTAWAY_ITEM] = function( _dpid )
    robot_trade.on_putaway_item(_dpid)
    return 0
end

g_func_map[ ROBOT_TAMERIEL_MATCH ] = function( _dpid ) 
    robot_tamriel_match.send_join_match( _dpid ) 
end



function handle_timer( _id, _p1, _p2, _index )
    local rt = 0
    local func = g_func_map[_id]
    if func then
        rt = func( _p1, _p2 )
    else
        c_err( "[timers](handle_timer) Unknown timer id: %d", _id )
    end
    return rt 
end

