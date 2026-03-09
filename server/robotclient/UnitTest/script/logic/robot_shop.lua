module( "robot_shop", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber
local mfloor, mrandom, mmin, ostime, osdifftime = math.floor, math.random, math.min, os.time, os.difftime

local function is_op_timeout( _hero, _last_op_time_name, _timeout_val )
    return not _hero[_last_op_time_name] or osdifftime( ostime(), _hero[_last_op_time_name] ) >= _timeout_val
end

function after_hero_added( _dpid, _hero )
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    local ctrl_id = _hero.ctrl_id_
    if dpid_info.dpid_type_ == robotclient.DPID_TYPE_GAME then
        g_timermng:c_add_timer_second( 2, timers.CREATE_SHOP, _dpid, ctrl_id, 10 )
        g_timermng:c_add_timer_second( 2, timers.CREATE_SHOP_GOLD_NOT_ENOUGH, _dpid, ctrl_id, 3 )
        g_timermng:c_add_timer_second( 2, timers.GET_SHOP_LIST, _dpid, ctrl_id, 5 )
        g_timermng:c_add_timer_second( 3, timers.GET_MY_SHOP, _dpid, ctrl_id, 3 )
      --  g_timermng:c_add_timer_second( 5, timers.GET_SHOP_BID_GOLD_INFO, _dpid, ctrl_id, 5 )      -- 商店已经取消掉竟价排名功能
        c_log( "[robot_shop] after_hero_added, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    end
end

function send_create_shop( _dpid, _ctrl_id )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return end

    -- shop type
    --local shop_type = hero.last_create_shop_type_ or 0
    --shop_type = shop_type % ( share_const.SHOP_TYPE_ALL - 1 ) + 1

    -- 商店现在不区分装备商店和杂货商店。统一为SHOP_TYPE_ALL.
    --hero.last_create_shop_type_ = shop_type
    local shop_type = share_const.SHOP_TYPE_ALL
    hero.last_create_shop_type_ = share_const.SHOP_TYPE_ALL

    -- shop name
    local shop_name = "我的商店" .. mrandom( 1 , 10000 )
    --if shop_type == share_const.SHOP_TYPE_EQUIP then
    --    shop_name = "我的装备商店"
    --else
    --    shop_name = "我的杂货商店"
    --end

    local shop_desc = "机器人商店"

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_CREATE_SHOP )
    ar:write_byte( shop_type )
    ar:write_string( shop_name )
    ar:write_string( shop_desc )
    ar:send_one_ar( g_robotclient, _dpid )
end

function timer_send_create_shop( _dpid, _ctrl_id )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return 0 end
    if hero.gold_ < share_const.SHOP_STARTUP_NEED_GOLD then return 1 end
    send_create_shop( _dpid, _ctrl_id )
    c_log( "[robot_shop](timer_send_create_shop) send create shop, dpid: 0x%X, player_id: %d", _dpid, hero.player_id_ )
    local shops = hero.shops_
    if shops and #shops >= share_const.SHOP_CNT_MAX_PER_PLAYER then
        c_log( "[robot_shop](timer_send_create_shop) max shop count reached, stop creating shops, _dpid: 0x%X, player_id: %d, ctrl_id: %d", _dpid, hero.player_id_, _ctrl_id )
        return 0
    end
    return 1
end

function timer_send_create_shop_gold_not_enough( _dpid, _ctrl_id )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return 0 end
    if hero.gold_ < share_const.SHOP_STARTUP_NEED_GOLD then
        send_create_shop( _dpid, _ctrl_id )
        c_log( "[robot_shop](timer_send_create_shop_gold_not_enough) send create shop when gold not enough, dpid: 0x%X, player_id: %d", _dpid, hero.player_id_ )
        return 0
    end
    return 1
end

function send_put_items_into_shop( _dpid, _shop_local_id, _items )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_PUT_ITEMS_INTO_SHOP )
    ar:write_int( _shop_local_id )
    ar:write_byte( #_items )
    for j, item in ipairs( _items ) do
        ar:write_int( item.index_ )
        ar:write_int( item.item_id_ )
        ar:write_int( item.item_num_ )
        ar:write_int( item.price_ )
    end
    ar:send_one_ar( g_robotclient, _dpid )
end

function send_random_put_items_into_shop( _dpid, _hero, _shop )
    local items_in_bag = _hero.bag_.item_list_
    if #items_in_bag <= 0 then return end

    local item_cnt_in_shop = #_shop.items_
    if item_cnt_in_shop < share_const.SHOP_CAPACITY then
        -- 先判断哪些物品是可以放进商店的，区分装备和非装备物品
        local items_can_be_put = {}
        --if _shop.shop_type_ == share_const.SHOP_TYPE_EQUIP then
            for index, item in ipairs( items_in_bag ) do
             --   if item_t.can_equip( item ) and item.is_equip_ == 0 then
                    tinsert( items_can_be_put, index )
             --   end
            end
       -- else
       --     for index, item in ipairs( items_in_bag ) do
       --         if not item_t.can_equip( item ) then
      --              tinsert( items_can_be_put, index )
       --         end
       --     end
     --   end

        -- 从背包里随机抽取几件物品放进商店
        if #items_can_be_put > 0 then
            local items_to_put = {}
            local item_cnt_to_put_max = mrandom( mmin( share_const.SHOP_CAPACITY - item_cnt_in_shop
                                                     , share_const.SHOP_PUT_ITEM_CNT_MAX
                                                     , #items_can_be_put
                                                     ) )
            local last_item_can_be_put_index = 0
            c_log( "[robot_shop](send_random_put_items_into_shop) put items begin "
                 .."dpid: 0x%X, player_id: %d, shop_local_id: %d, shop_type: %d"
                 , _dpid, _hero.player_id_, _shop.shop_local_id_, _shop.shop_type_
                 )
            for j = 1, item_cnt_to_put_max do
                last_item_can_be_put_index = mrandom( last_item_can_be_put_index + 1, #items_can_be_put )
                local item_index = items_can_be_put[last_item_can_be_put_index]
                local item = items_in_bag[item_index]
                local item_to_put =
                {
                    index_ = item_index,
                    item_id_ = item.item_id_,
                    item_num_ = mrandom( item.item_num_ ),
                    price_ = mrandom( 999 ),
                }
                tinsert( items_to_put, item_to_put )
                c_log( "[robot_shop](send_random_put_items_into_shop) put item, "
                     .."index: %d, item_id: %d, item_num: %d, price: %d"
                     , item_to_put.index_
                     , item_to_put.item_id_
                     , item_to_put.item_num_
                     , item_to_put.price_
                     )
                if last_item_can_be_put_index >= item_cnt_to_put_max then break end
            end

            send_put_items_into_shop( _dpid, _shop.shop_local_id_, items_to_put )

            c_log( "[robot_shop](send_random_put_items_into_shop) put items end"
                 .."dpid: 0x%X, player_id: %d, shop_local_id: %d"
                 , _dpid, _hero.player_id_, _shop.shop_local_id_
                 )

            return #items_to_put
        end
    end
    return 0
end

function send_withdraw_shop_item( _dpid, _shop_local_id, _item_sn_in_shop )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_WITHDRAW_SHOP_ITEM )
    ar:write_int( _shop_local_id )
    ar:write_double( _item_sn_in_shop )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_shop](send_withdraw_shop_item) dpid: 0x%X, shop_local_id: %d, item_sn_in_shop: %.0f"
         , _dpid, _shop_local_id, _item_sn_in_shop
         )
end

function timer_get_shop_list( _dpid )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return 0 end
    
    local shop_type = hero.last_get_shop_list_shop_type_ or 0
    shop_type = shop_type % share_const.SHOP_TYPE_ALL + 1
    hero.last_get_shop_list_shop_type_ = shop_type

    local sort_type = mrandom( 2 )

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_GET_SHOP_LIST )
    ar:write_byte( shop_type )
    ar:write_byte( sort_type )
    if sort_type == share_const.SHOP_LIST_SORTED then
        ar:write_int( mrandom( 10 ) )  -- 页码
    end
    ar:send_one_ar( g_robotclient, _dpid )

    c_log( "[robot_shop](timer_get_shop_list) dpid: 0x%X, player_id: %d, shop_type: %d, sort_type: %d"
         , _dpid, hero.player_id_, shop_type, sort_type
         )

    return 1
end

function send_withdraw_shop_gold( _dpid, _shop_local_id, _gold )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_WITHDRAW_SHOP_GOLD )
    ar:write_int( _shop_local_id )
    ar:write_int( _gold )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_shop](send_withdraw_shop_gold) dpid: 0x%X, shop_local_id: %d, gold: %d", _dpid, _shop_local_id, _gold )
end

function send_put_gold_into_shop( _dpid, _shop_local_id, _gold )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_PUT_GOLD_INTO_SHOP )
    ar:write_int( _shop_local_id )
    ar:write_int( _gold )
    ar:send_one_ar( g_robotclient, _dpid )
    c_log( "[robot_shop](send_put_gold_into_shop) dpid: 0x%X, shop_local_id: %d, gold: %d", _dpid, _shop_local_id, _gold )
end

function timer_get_my_shop( _dpid, _ctrl_id )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return 0 end
    local ar = robotclient.g_ar
    for i, shop in ipairs( hero.shops_ ) do
        ar:flush_before_send( msg.PT_GET_SHOP )
        ar:write_int( shop.shop_server_id_ )
        ar:write_int( shop.shop_local_id_ )
        ar:send_one_ar( g_robotclient, _dpid )
    end
    return 1
end

function send_add_shop_bid_gold( _dpid, _shop_local_id, _gold )
  --  local ar = robotclient.g_ar
  --  ar:flush_before_send( msg.PT_ADD_SHOP_BID_GOLD )
  --  ar:write_int( _shop_local_id )
  --  ar:write_int( _gold )
  --  ar:send_one_ar( g_robotclient, _dpid )
  --  c_log( "[robot_robot](send_add_shop_bid_gold) dpid: 0x%X, shop_local_id: %d, gold: %d", _dpid, _shop_local_id, _gold )
end

function timer_get_shop_bid_gold_info( _dpid, _ctrl_id )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return 0 end
    for i, shop in ipairs( hero.shops_ ) do
        local ar = robotclient.g_ar
        ar:flush_before_send( msg.PT_GET_SHOP_BID_GOLD_INFO )
        ar:write_int( shop.shop_local_id_ )
        ar:send_one_ar( g_robotclient, _dpid )
    end
    c_log( "[robot_robot](timer_get_shop_bid_gold_info) player_id: %d", hero.player_id_ )
    return 1
end

function on_shop( _ar, _dpid, _size )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return end

    local shop_server_id = _ar:read_int()
    local shop_local_id = _ar:read_int()
    local shop_type = _ar:read_byte()
    local shop_name = _ar:read_string()
    local shop_desc = _ar:read_string()
    local shop_owner_player_id = _ar:read_int()
    c_log( "[robot_shop](on_shop) shop received, dpid: 0x%X, player_id: %d, "
         .."shop_server_id: %d, shop_local_id: %d, shop_name: %s, shop_owner_player_id: %d"
         , _dpid, hero.player_id_, shop_server_id, shop_local_id, shop_name, shop_owner_player_id
         )
    local head_icon_id = _ar:read_int()
    local gold = _ar:read_int()
    local bind_gold = _ar:read_int()
    local bid_gold = _ar:read_int()
    local bid_gold_times = _ar:read_byte()
    local items = {}
    local item_cnt = _ar:read_ushort()
    local unserialize_item_for_client = item_t.unserialize_item_for_client
    for i = 1, item_cnt do
        local item = unserialize_item_for_client( _ar )
        item.item_sn_in_shop_ = _ar:read_double()
        item.price_ = _ar:read_int()
        tinsert( items, item )
        c_log( "[robot_shop](on_shop) shop item received, item id: %d, item num: %d, item_sn_in_shop: %.0f, price: %d"
             , item.item_id_, item.item_num_, item.item_sn_in_shop_, item.price_ )
    end

    local shop =
    {
        shop_server_id_ = shop_server_id,
        shop_local_id_ = shop_local_id,
        shop_name_ = shop_name,
        shop_type_ = shop_type,
        player_id_ = shop_owner_player_id,
        gold_ = gold,
        bid_gold_ = bid_gold,
        bid_gold_times_ = bid_gold_times,
        items_ = items,
    }

    -- 判断是否自己的商店
    local is_my_shop = false
    for i, my_shop_info in ipairs( hero.shops_ ) do
        if my_shop_info.shop_server_id_ == shop_server_id and my_shop_info.shop_local_id_ == shop_local_id then
            is_my_shop = true
            break
        end
    end

    if is_my_shop then
        local ostime_now = ostime()

        c_log( "[robot_shop](on_shop) my shop updated, dpid: 0x%X, player_id: %d, shop_server_id: %d, shop_local_id: %d"
             , _dpid, hero.player_id_, shop_server_id, shop_local_id
             )

        -- 随机下架一件商品
        if #items > 0 and is_op_timeout( hero, "last_withdraw_shop_item_time_", 14 ) then
            send_withdraw_shop_item( _dpid, shop_local_id, items[mrandom( #items )].item_sn_in_shop_ )
            hero.last_withdraw_shop_item_time_ = ostime_now
        end

        if is_op_timeout( hero, "last_rm_bag_item_time_", 7 ) and send_random_put_items_into_shop( _dpid, hero, shop ) > 0 then
            hero.last_rm_bag_item_time_ = ostime_now
        end

        -- 存钱
        if gold <= limit.INT_MAX - 100 and is_op_timeout( hero, "last_put_gold_into_shop_time_", 14 ) then
            send_put_gold_into_shop( _dpid, shop_local_id, mrandom( mmin( hero.gold_, 99 ) ) )
            hero.last_put_gold_into_shop_time_ = ostime_now
        end

        -- 取钱
        if gold > 0 and is_op_timeout( hero, "last_withdraw_shop_gold_time_", 14 ) then
            send_withdraw_shop_gold( _dpid, shop_local_id, mrandom( mmin( gold, 99 ) ) )
            hero.last_withdraw_shop_gold_time_ = ostime_now
        end
    else
        -- 随便买一件商品
        for index, item in ipairs( items ) do
            if hero.gold_ > item.price_ then
                local item_num = mrandom( mmin( mfloor( hero.gold_ / item.price_ ), item.item_num_ ) )
                local ar = robotclient.g_ar
                ar:flush_before_send( msg.PT_BUY_ITEM_IN_SHOP )
                ar:write_int( shop_server_id )
                ar:write_int( shop_local_id )
                ar:write_double( item.item_sn_in_shop_ )
                ar:write_int( item.item_id_ )
                ar:write_int( item_num )
                ar:send_one_ar( g_robotclient, _dpid )
                c_log( "[robot_shop](on_shop) buy item, player_id: %d, "
                     .."shop_server_id: %d, shop_local_id: %d, item_sn_in_shop: %.0f, item_num: %d"
                     , hero.player_id_, shop_server_id, shop_local_id, item.item_sn_in_shop_, item_num
                     )
                break
            end
        end
    end
end

function on_shop_list( _ar, _dpid, _size )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return end
    local shop_type = _ar:read_byte()
    local sort_type = _ar:read_byte()
    local page = _ar:read_int()
    local max_page = _ar:read_int()
    local shop_cnt = _ar:read_byte()
    local shop_list = {}
    c_log( "[robot_shop](on_shop_list) dpid: 0x%X, player_id: %d, shop list begin", _dpid, hero.player_id_ )
    for i = 1, shop_cnt do
        local shop =
        {
            shop_server_id_ = _ar:read_int(),
            shop_local_id_ = _ar:read_int(),
            head_icon_id_ = _ar:read_int(),
            shop_type_ = _ar:read_byte(),
            shop_name_ = _ar:read_string(),
            shop_desc_ = _ar:read_string(),
        }
        tinsert( shop_list, shop )
        c_log( "[robot_shop](on_shop_list) shop_server_id: %d, shop_local_id: %d, shop_type: %d, shop_name: %s"
             , shop.shop_server_id_, shop.shop_local_id_, shop.shop_type_, shop.shop_name_
             )
    end
    c_log( "[robot_shop](on_shop_list) dpid: 0x%X, player_id: %d, shop list end", _dpid, hero.player_id_ )

    if #shop_list > 0 then
        -- get shop
        local shop = shop_list[mrandom( #shop_list )]
        local ar = robotclient.g_ar
        ar:flush_before_send( msg.PT_GET_SHOP )
        ar:write_int( shop.shop_server_id_ )
        ar:write_int( shop.shop_local_id_ )
        ar:send_one_ar( g_robotclient, _dpid )
    end
end

function on_shop_operate_fail( _ar, _dpid, _size )
    c_log( "[robot_shop](on_shop_operate_fail) dpid: 0x%X", _dpid )
end

function on_my_shops( _ar, _dpid, _size )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return end
    player_t.unserialize_my_shops_for_client( hero, _ar )
end

function on_shop_bid_gold_info( _ar, _dpid, _size )
    local hero = robotclient.get_hero( _dpid )
    if not hero then return end
    local shop_local_id = _ar:read_int()
    local bid_gold_today = _ar:read_int()
    local bid_gold_times = _ar:read_byte()
    local rank_tomorrow = _ar:read_int()
    c_log( "[robot_shop](on_shop_bid_gold_info) player_id: %d, shop_local_id: %d", hero.player_id_, shop_local_id )
    -- 出价竞拍排位
    if bid_gold_times < share_const.SHOP_BID_GOLD_TIMES_MAX then
        send_add_shop_bid_gold( _dpid, shop_local_id, mrandom( mmin( hero.gold_, share_const.SHOP_BID_GOLD_MAX_EACH_TIME ) ) )
    end
end

