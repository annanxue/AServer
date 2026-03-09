module( "shop_mng", package.seeall )

local sformat, tinsert, unpack = string.format, table.insert, unpack

local INIT_STATE_UNINIT = 1
local INIT_STATE_SENT_QUERY = 2
local INIT_STATE_RECVING_RESULT = 3
local INIT_STATE_INITED = 4

g_init_state = g_init_state or INIT_STATE_UNINIT

-- 为了避免在物品流通期间，商店正好赶上维护时间且由于资金不足而被关闭，我们需要在商店
-- 维护前给所有商店上锁，不允许玩家操作商店及里面的物品。而在商品维护完毕后，也需要等game
-- 和bf的数据都同步好，才解锁。
g_locked_for_maintain = g_locked_for_maintain or false

g_locked_by_gm = g_locked_by_gm or false

function is_shop_mng_ready()
    return g_init_state == INIT_STATE_INITED and not g_locked_for_maintain and not g_locked_by_gm and
           game_zone_mng.g_server_list and game_zone_mng.g_zone_list
end

function try_query_all_shops()
    if g_init_state == INIT_STATE_UNINIT and g_dbclient:c_is_connected() and
       ( not g_gamesvr:c_get_allow_shops_on_bf() or bfclient.g_is_zone_info_loaded_on_bf[0] ) then
        dbclient.remote_call_db( "db_shop.query_all_shops" )
        g_init_state = INIT_STATE_SENT_QUERY
        c_log( "[shop_mng](try_query_all_shops) send query to db, g_init_state = INIT_STATE_SENT_QUERY" )
    end
end

function on_query_result_begin()
    if g_init_state ~= INIT_STATE_SENT_QUERY then
        c_err( "[shop_mng](on_query_result_begin) g_init_state: %d, not INIT_STATE_SENT_QUERY", g_init_state )
        return
    end
    g_init_state = INIT_STATE_RECVING_RESULT
    remote_call_bf( "shop_mng.clear_shops_of_game", g_gamesvr:c_get_server_id() )
    c_log( "[shop_mng](on_query_result_begin) g_init_state = INIT_STATE_RECVING_RESULT" )
end

function on_shop_queried( _shop )
    if g_init_state ~= INIT_STATE_RECVING_RESULT then
        c_err( "[shop_mng](on_shop_queried) g_init_state: %d, not INIT_STATE_RECVING_RESULT, discard the query result", g_init_state )
        return
    end
    remote_call_bf( "shop_mng.init_shop_of_game", _shop )
    c_log( "[shop_mng](on_shop_queried) shop_local_id: %d", _shop.shop_local_id_ )
end

function on_query_result_end()
    if g_init_state ~= INIT_STATE_RECVING_RESULT then
        c_err( "[shop_mng](on_query_result_end) g_init_state: %d, not INIT_STATE_RECVING_RESULT", g_init_state )
        return
    end
    g_init_state = INIT_STATE_INITED
    remote_call_bf( "shop_mng.on_init_shops_of_game_done", g_gamesvr:c_get_server_id() )
    c_log( "[shop_mng](on_query_result_end) all shops received, g_init_state = INIT_STATE_INITED" )
end

function remote_call_bf( _func_name, ... )
    if g_gamesvr:c_get_allow_shops_on_bf() then
        bfclient.remote_call_center( _func_name, ... )
    else
        local_call( _func_name, ... )
    end
end

function on_bf_load_zone_info( _bf_id )
    if _bf_id == 0 and g_gamesvr:c_get_allow_shops_on_bf() then
        try_query_all_shops()
    end
end

function on_bf_disconnect( _bf_id )
    if _bf_id == 0 and g_gamesvr:c_get_allow_shops_on_bf() then
        g_init_state = INIT_STATE_UNINIT
        c_log( "[shop_mng](on_bf_disconnect) center bf disconnect, g_init_state set to INIT_STATE_UNINIT" )
    end
end

function lock_all_shops()
    g_locked_for_maintain = true
    c_log( "[shop_mng](lock_all_shops) lock all shops for maintain" )
end

function unlock_all_shops()
    g_locked_for_maintain = false
    c_log( "[shop_mng](unlock_all_shops) unlock all shops after maintain" )
end

function send_shop_to_client( _player_id, _shop )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player.shop_to_sent_ = _shop
        -- 等1帧再发ST_SHOP，这样可以保证客户端在收到ST_RM_ITEM等snapshop包，刷新背包之后，才收到ST_SHOP，关闭WaitingUI。
        -- 客户端的很多逻辑都是收到ST_SHOP就关闭WaitingUI。
        g_timermng:c_add_timer( 1, timers.SEND_SHOP_TO_CLIENT, _player_id, 0, 0 )
    end
end

function timer_send_shop_to_client( _player_id )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player and player.shop_to_sent_ then
        player:add_shop( player.shop_to_sent_ )
    end
end

function on_shop_created( _shop )
    remote_call_bf( "shop_mng.add_shop", _shop )

    local player_id = _shop.player_id_
    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        local shop_info =
        {
            shop_local_id_ = _shop.shop_local_id_,
            shop_name_ = _shop.shop_name_,
            shop_type_ = _shop.shop_type_
        }
        tinsert( player.shops_, shop_info )
        player:add_my_shops()
        player:add_shop( _shop )
    end

    c_log( "[shop_mng](on_shop_created) player_id: %d, shop_local_id: %d", player_id, _shop.shop_local_id_ )
end

function on_put_items_into_shop_success( _shop )
    dbclient.send_save_bag_shop_items_to_db( _shop )
    send_shop_to_client( _shop.player_id_, _shop )
    c_log( "[shop_mng](on_put_items_into_shop_success) player_id: %d, shop_local_id: %d", _shop.player_id_, _shop.shop_local_id_ )
end

function send_shop_list_to_client( _player_id, _shop_type, _sort_type, _page, _max_page,  _shop_list )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player:add_shop_list( _shop_type, _sort_type, _page, _max_page, _shop_list )
    end
end

function send_favorite_shop_id_list_to_client( _player_id, _favorite_shops )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player.favorite_shops_ = _favorite_shops
        player:add_favorite_shop_id_list()
    end
end

function send_favorite_shops_to_client( _player_id, _favorite_shops )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player:add_favorite_shop_info_list( _favorite_shops )
    end
end

function send_shop_operate_fail_to_client( _player_id, _msg, _msg_args )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        if _msg_args then
            player:add_defined_text_str( _msg, unpack( _msg_args ) )
        else
            player:add_defined_text( _msg )
        end
        player:add_shop_operate_fail()
    end
end

function consume_gold_on_buyer( _buyer_player_id, _shop_server_id, _shop_local_id, _item_sn_in_shop, _lock_id, _price )
    local player = player_mng.get_player_by_player_id( _buyer_player_id )
    if not player then
        remote_call_bf( "shop_mng.unlock_item", _lock_id, true )
        c_log( "[shop_mng](consume_gold_on_buyer) buyer offline, buyer_player_id: %d", _buyer_player_id )
        return
    end
    player:consume_gold_as_shop_item_buyer( _shop_server_id, _shop_local_id, _item_sn_in_shop, _lock_id, _price )
end

function fetch_item_for_buyer( _shop_server_id, _shop_local_id, _item_sn_in_shop, _buyer_player_id )
    remote_call_bf( "shop_mng.do_fetch_item_for_buyer"
                  , _shop_server_id
                  , _shop_local_id
                  , _item_sn_in_shop
                  , g_gamesvr:c_get_server_id()
                  , _buyer_player_id
                  )
    c_log( "[shop_mng](fetch_item_for_buyer) shop_server_id: %d, shop_local_id: %d, item_sn_in_shop: %.0f, buyer_player_id: %d"
         , _shop_server_id, _shop_local_id, _item_sn_in_shop, _buyer_player_id
         )
end

function save_shop_on_sell( _params )
    dbclient.send_save_shop_to_db_on_sell( _params )
    c_log( "[shop_mng](save_shop_on_sell) shop_local_id: %d, sold_item: %s, buyer_server_id: %d, buyer_player_id: %d"
         , _params.shop_local_id_, _params.sold_item_str_, _params.buyer_server_id_, _params.buyer_player_id_ )
end

function give_item_to_buyer_from_db( _shop_local_id, _item_str, _buyer_server_id, _buyer_player_id )
    remote_call_bf( "shop_mng.give_item_to_buyer", g_gamesvr:c_get_server_id(), _shop_local_id, _item_str, _buyer_server_id, _buyer_player_id )
    c_log( "[shop_mng](give_item_to_buyer_from_db) shop_local_id: %d, buyer_server_id: %d, buyer_player_id: %d, item_str: %s"
         , _shop_local_id, _buyer_server_id, _buyer_player_id, _item_str )
end

function do_give_item_to_buyer( _item_str, _buyer_player_id, _shop )
    local item_tbl = loadstring( "return " .. _item_str )()
    local item = item_t.unserialize_item_from_peer( { player_id_ = _buyer_player_id }, item_tbl, "[BUY_FROM_SHOP]" )
    item:update_bind( { player_id_ = _buyer_player_id } )
    local player = player_mng.get_player_by_player_id( _buyer_player_id )
    if player then
        local item_list = { item }
        player:pick_item_list_safe_with_trade_bind( item_list, "[BUY_ITEM_IN_SHOP]" )
        player:add_defined_text_str( MESSAGES.PLAYER_SHOP_BUY_ITEM_SUCC, {share_const.ITEM_ID, item.item_id_}, {share_const.RAW_TEXT, item.item_num_} )
        -- 如果商品从商店拿出后，bf和卖家game的连接断开，发货时就会找不到卖家的商店，此时_shop就会为空，
        -- 见luacommon/object/shop_mng.lua, give_item_to_buyer函数。
        if _shop then
            send_shop_to_client( _buyer_player_id, _shop )
        end
    else
        local trade_item = item_t.get_trade_bind_item( item )
        local item_list = { trade_item }
        mail_box_t.add_mail( _buyer_player_id, 9, item_list )
    end
    c_log( "[shop_mng](do_give_item_to_buyer) buyer_player_id: %d, buyer_online: %s, item_id: %d, item_num: %d, item_sn: %d"
         , _buyer_player_id, player and "true" or "false", item.item_id_, item.item_num_, item.item_sn_ )
end

function remove_shop_in_db( _shop_local_id, _player_id, _items, _bind_gold, _mail_id )
    dbclient.send_delete_shop( _shop_local_id, _player_id )
    if _bind_gold > 0 then
        local bind_gold_item = item_t.new_by_system( share_const.BIND_GOLD_ITEM_ID, _bind_gold, "[REMOVE_SHOP_IN_DB]" )
        tinsert( _items, bind_gold_item )
    end
    mail_box_t.add_mail( _player_id, _mail_id, _items )
    c_log( "[shop_mng](remove_shop_in_db) shop_local_id: %d, player_id: %d", _shop_local_id, _player_id )
end

function update_player_shops( _player_id, _shops )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player.shops_ = _shops
        player:add_my_shops()
        c_log( "[shop_mng](update_player_shops) shops updated, player_id: %d, shop count: %d", _player_id, #_shops )
    end
end

function on_withdraw_shop_gold( _shop, _withdrawn_bind_gold, _withdrawn_gold )
    local player_id = _shop.player_id_
    dbclient.remote_call_db( "db_shop.update_shop_gold", _shop.shop_local_id_, _shop.bind_gold_, _shop.gold_ )
    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        if _withdrawn_bind_gold > 0 then
            player:add_bind_gold( _withdrawn_bind_gold, "[WITHDRAW_SHOP_GOLD]" )
        end
        if _withdrawn_gold > 0 then
            player:add_non_bind_gold( _withdrawn_gold, "[WITHDRAW_SHOP_GOLD]" )
        end
        player:add_defined_text_str( MESSAGES.PLAYER_SHOP_WITHDRAW_GOLD_SUCC, _withdrawn_bind_gold + _withdrawn_gold )
        send_shop_to_client( player_id, _shop )
        -- 因为db_shop.update_shop_gold已经在数据库中将商店资金减少，所以这里将玩家在数据库中的金币数量也刷新一下，
        -- 可防止玩家存盘前，db/game之间的连接断开导致的金币丢失。
        dbclient.remote_call_db( "db_login.update_player_bind_gold", player_id, player.bind_gold_ )
        dbclient.remote_call_db( "db_login.update_player_gold", player_id, player.gold_ )
    else
        local bind_gold_item = item_t.new_by_system( share_const.BIND_GOLD_ITEM_ID, _withdrawn_bind_gold, "[WITHDRAW_SHOP_GOLD]" )
        local gold_item = item_t.new_by_system( share_const.GOLD_ITEM_ID, _withdrawn_gold, "[WITHDRAW_SHOP_GOLD]" )
        mail_box_t.add_mail( player_id, 11, { bind_gold_item, gold_item } )
    end
end

function withdraw_shop_gold_on_close_shop( _player_id, _withdrawn_bind_gold, _withdrawn_gold )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        if _withdrawn_bind_gold > 0 then
            player:add_bind_gold( _withdrawn_bind_gold, "[CLOSE_SHOP]" )
        end
        if _withdrawn_gold > 0 then
            player:add_non_bind_gold( _withdrawn_gold, "[CLOSE_SHOP]" )
        end
        player:add_defined_text( MESSAGES.PLAYER_SHOP_CLOSE_SHOP_SUCCEED )
        -- 因为在该函数之前，商店已经从数据库中删除，所以这里将玩家在数据库中的金币数量也刷新一下，
        -- 可防止玩家存盘前，db/game之间的连接断开导致的金币丢失。
        dbclient.remote_call_db( "db_login.update_player_bind_gold", _player_id, player.bind_gold_ )
        dbclient.remote_call_db( "db_login.update_player_gold", _player_id, player.gold_ )
        c_log( "[shop_mng](withdraw_shop_gold_on_close_shop) player online, player_id: %d", _player_id )
    else
        local bind_gold_item = item_t.new_by_system( share_const.BIND_GOLD_ITEM_ID, _withdrawn_bind_gold, "[WITHDRAW_SHOP_GOLD]" )
        local gold_item = item_t.new_by_system( share_const.GOLD_ITEM_ID, _withdrawn_gold, "[CLOSE_SHOP]" )
        mail_box_t.add_mail( _player_id, 11, { bind_gold_item, gold_item } )
        c_log( "[shop_mng](withdraw_shop_gold_on_close_shop) player offline, player_id: %d", _player_id )
    end
end

function on_put_gold_into_shop( _shop, _gold )
    local player_id = _shop.player_id_
    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        dbclient.remote_call_db( "db_login.update_player_gold", player_id, player.gold_ )  -- 即刻存盘，避免金币在数据库中被复制
        send_shop_to_client( player_id, _shop )
        player:add_defined_text_str( MESSAGES.PLAYER_SHOP_PUT_GOLD_SUCC, _gold )
    end
    dbclient.remote_call_db( "db_shop.update_shop_gold", _shop.shop_local_id_, _shop.bind_gold_, _shop.gold_ )
end

function on_get_info_to_withdraw_shop_item( _player_id, _shop_local_id, _item_sn_in_shop, _item_id, _item_num )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        if player:is_full_with_item( _item_id, _item_num ) then
            send_shop_operate_fail_to_client( _player_id, MESSAGES.PLAYER_SHOP_BAG_IS_FULL_CANT_FETCH_ITEM )
            c_log( "[shop_mng](on_get_info_to_withdraw_shop_item) bag is full, player_id: %d, item_id: %d, item_num to pick: %d"
                 , _player_id, _item_id, _item_num
                 )
            return
        end
        remote_call_bf( "shop_mng.withdraw_shop_item", g_gamesvr:c_get_server_id(), _shop_local_id, _player_id, _item_sn_in_shop )
    end
end

function on_withdraw_shop_item( _shop, _item )
    setmetatable( _item, item_t.item_mt )  -- bf送过来的table没有元表，故需要设置
    local item_list = { _item }
    local player_id = _shop.player_id_ 
    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        player:pick_item_list_safe( item_list, "[WITHDRAW_SHOP_ITEM]" )
        player:add_defined_text_str( MESSAGES.PLAYER_SHOP_WITHDRAW_ITEM_SUCC, {share_const.ITEM_ID, _item.item_id_}, {share_const.RAW_TEXT, _item.item_num_} )
    else
        mail_box_t.add_mail( player_id, 12, item_list )
    end
    dbclient.send_save_bag_shop_items_to_db( _shop )
    send_shop_to_client( _shop.player_id_, _shop )
    c_log( "[shop_mng](on_withdraw_shop_item) shop_local_id: %d, player_id: %d, item_id: %d, item_num: %d"
         , _shop.shop_local_id_, player_id, _item.item_id_, _item.item_num_
         )
end

function on_add_shop_bid_gold( _player_id, _shop_local_id, _bid_gold_today, _bid_gold_times )
    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        dbclient.remote_call_db( "db_login.update_player_gold", _player_id, player.gold_ )  -- 即刻存盘，避免金币在数据库中被复制
    end
    dbclient.remote_call_db( "db_shop.update_shop_bid_gold", _shop_local_id, _bid_gold_today, _bid_gold_times )
end

function on_get_shop_bid_gold_info( _player_id, _shop_local_id, _bid_gold_today, _bid_gold_times, _rank_tomorrow )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player:add_shop_bid_gold_info( _shop_local_id, _bid_gold_today, _bid_gold_times, _rank_tomorrow )
    end
end

function on_shop_name_desc_modified( _shop )
    local player_id = _shop.player_id_
    local shop_local_id = _shop.shop_local_id_
    local shop_name = _shop.shop_name_
    dbclient.remote_call_db( "db_shop.update_shop_name_desc", shop_local_id, player_id, shop_name, _shop.shop_desc_ )
    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        for i, shop in ipairs( player.shops_ ) do
            if shop.shop_local_id_ == shop_local_id then
                shop.shop_name_ = shop_name
                break
            end
        end
        player:add_my_shops()
        player:add_shop( _shop )
    end
end

function on_shop_item_price_modified( _shop )
    -- 同时存盘并刷新客户端，不必等待存盘成功。
    -- 万一存盘失败，那么因为bf上已经采用了新的价格，所以其他玩家可能已经以新价格请求购买该商品，这时如果玩家购买成功，商店还会存一次盘；
    -- 如果第二次的存盘成功，那么上次的存盘错误就弥补回来了；如果第二次存盘也失败了，那么玩家对该物品的购买流程也会失败，不用担心玩家
    -- 以新价格买走了该物品。虽然客户端那边已经将物品价格显示为新的，但这也不会给玩家造成困惑（因为客户端表现上并没有数据不一致的情况）。
    -- 最严谨的做法可能是给物品上锁，等db存盘成功后才解锁，和购买流程类似。但这个实现方案较复杂，数据流是
    --
    --   client->game->bf->game->db->game->bf->game->client
    --
    -- 但是售价修改流程并不像购买流程那样要跨越两个game，也不存在物品和金钱被复制的问题，所以并不需要采用如此复杂的流程。
    dbclient.send_save_shop_items_to_db( _shop )
    send_shop_to_client( _shop.player_id_, _shop )
end

function on_player_change_head_icon( _player )
    local player_id = _player.player_id_
    local head_icon_id = _player.head_icon_id_
    for i, shop_info in ipairs( _player.shops_ ) do
        local shop_local_id = shop_info.shop_local_id_
        remote_call_bf( "shop_mng.update_shop_head_icon", g_gamesvr:c_get_server_id(), shop_local_id, player_id, head_icon_id )
        dbclient.remote_call_db( "db_shop.update_shop_head_icon", shop_local_id, head_icon_id )
        c_log( "[shop_mng](on_player_change_head_icon) player_id: %d, shop_local_id: %d, head_icon_id: %d"
             , player_id, shop_local_id, head_icon_id )
    end
end
