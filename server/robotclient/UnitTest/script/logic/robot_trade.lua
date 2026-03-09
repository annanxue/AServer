module( "robot_trade", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber

ITEM_LIST_TYPE_EQUIP    = 1
ITEM_LIST_TYPE_SKILL    = 2
ITEM_LIST_TYPE_SOUL     = 3
ITEM_LIST_TYPE_RUNE     = 4
ITEM_LIST_TYPE_MAX      = 4

local function is_op_timeout( _hero, _last_op_time_name, _timeout_val )
    return not _hero[_last_op_time_name] or osdifftime( ostime(), _hero[_last_op_time_name] ) >= _timeout_val
end

function after_hero_added( _dpid, _hero )
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    local ctrl_id = _hero.ctrl_id_
    robotclient.send_robot_add_item(_dpid)
    if dpid_info.dpid_type_ == robotclient.DPID_TYPE_GAME then
        g_timermng:c_add_timer_second( 10, timers.PUTAWAY_ITEM, _dpid, ctrl_id, 0 )
        c_log( "[robot_shop] after_hero_added, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    end
end

local function is_item_can_been_putaway(_item)
    local list_type
    if _item:can_equip() then
        list_type = ITEM_LIST_TYPE_EQUIP 
    elseif _item.use_type_ == item_t.ITEM_USE_TYPE_SOUL then
        list_type =  ITEM_LIST_TYPE_SOUL
    elseif _item.use_type_ == item_t.ITEM_USE_TYPE_RUNE_MATERIAL then
        list_type = ITEM_LIST_TYPE_RUNE
    elseif _item.use_type_ == item_t.ITEM_USE_TYPE_SKILL_BOOK then
        list_type = ITEM_LIST_TYEP_SKILL
    end

    if not list_type then
        return false
    end

    if _item:is_bind() then
        return false
    end

    return true
end

function on_putaway_item( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return end

    local bag_item_list = hero.bag_.item_list_
    if #bag_item_list <= 0 then return end
    local item_list = {}
    local count = 0
    for idx, item in pairs(bag_item_list) do
        if is_item_can_been_putaway(item) then
            item_list[idx] = item
            count = count + 1
            if count >= 30 then
                break
            end
        end
    end
    
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_PUTAWAY_ITEM )
    ar:write_byte(count)
    for idx, item in pairs(item_list) do
        ar:write_int( item.item_id_ )
        ar:write_short( idx )
        ar:write_short( item.item_num_)
        ar:write_int( math.random(100000))
    end
    ar:send_one_ar( g_robotclient, _dpid )
    return 0
end

