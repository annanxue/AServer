module( "robot_chat", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber
local mfloor, mrandom, mmin, ostime, osdifftime = math.floor, math.random, math.min, os.time, os.difftime

function after_hero_added( _dpid, _hero )
    local ctrl_id = _hero.ctrl_id_
    robot_gm.level( _hero, 10 )
    local interval = g_timermng:c_get_frame_rate() * 5
    g_timermng:c_add_timer( interval, timers.CHAT, _dpid, ctrl_id, interval ) 
    c_log( "[robot_chat] after_hero_added, start random chat, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
end

function timer_send_chat_msg( _dpid, _ctrl_id )
    local hero = ctrl_mng.get_ctrl( _dpid, _ctrl_id )
    if not hero then
        return 0
    end
    send_chat_msg( _dpid, hero )
    return 1
end

function send_chat_msg( _dpid, _hero )
    local item_str = ""
    local item_list = _hero.bag_.item_list_
    local item_count = mrandom( 2 )
    for i = 1, item_count do
        local item_index = mrandom( #item_list )
        local item_id = item_list[item_index].item_id_
        local item_name = ITEMS[item_id].Name
        item_str = item_str .. sformat( "[A2FFBD][u][url=sitem:%d][%s][/url][/u][-]", item_index, item_name )
    end
    local chat_msg = sformat( "玩家ID: %d, 聊天测试: %s", _hero.player_id_, item_str )
    c_log( chat_msg )

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_CHAT_MSG )
    ar:write_string( chat_msg )
    ar:write_int( 0 ) -- voice length
    ar:write_byte( 2 ) -- channel id: CHANNAL_ID_WORLD
    ar:write_int( 0 ) -- to player id
    ar:send_one_ar( g_robotclient, _dpid )
end
