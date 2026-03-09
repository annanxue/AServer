module( "gold_exchange_mng", package.seeall )

local sformat, tinsert = string.format, table.insert

local INIT_STATE_UNINIT         = 1
local INIT_STATE_SENT_QUERY     = 2
local INIT_STATE_RECVING_RESULT = 3
local INIT_STATE_INITED         = 4

g_init_state = g_init_state or INIT_STATE_UNINIT
g_is_allow_all_bf = g_is_allow_all_bf or true 

local function remote_call_bf( _func_name, ... )
    if g_is_allow_all_bf then
        bfclient.remote_call_center( _func_name, ... )
    else
        local_call( _func_name, ... )
    end
end

function is_gold_exchange_mng_ready()
    return g_init_state == INIT_STATE_INITED
end

function try_query_all_gold_exchanges()
    local log_head = "[gold_exchange_mng](try_query_all_gold_exchanges)"

    if g_init_state == INIT_STATE_UNINIT and g_dbclient:c_is_connected() and
       ( not g_is_allow_all_bf or bfclient.g_is_zone_info_loaded_on_bf[0] ) then
        dbclient.remote_call_db( "db_gold_exchange.query_all_gold_exchanges" )
        g_init_state = INIT_STATE_SENT_QUERY
        c_log( "%s send query to db, g_init_state: %d", log_head, g_init_state )
    end
end

function on_query_result_begin()
    local log_head = "[gold_exchange_mng](on_query_result_begin)"

    if g_init_state ~= INIT_STATE_SENT_QUERY then
        c_err( "%s g_init_state: %d, not INIT_STATE_SENT_QUERY", log_head, g_init_state )
        return
    end
    g_init_state = INIT_STATE_RECVING_RESULT
    remote_call_bf( "gold_exchange_mng.clear_gold_exchanges_of_game", g_gamesvr:c_get_server_id() )
    c_log( "%s recving result, g_init_state = %d", log_head, g_init_state )
end

function on_gold_exchange_queried( _gold_exchange )
    local log_head = "[gold_exchange_mng](on_gold_exchange_queried)"

    if g_init_state ~= INIT_STATE_RECVING_RESULT then
        c_err( "%s g_init_state: %d, not INIT_STATE_RECVING_RESULT, discard the query result", log_head, g_init_state )
        return
    end
    --remote_call_bf( "gold_exchange_mng.add_gold_exchange", _gold_exchange )
    ---[[
    dbclient.remote_call_db( "db_gold_exchange.save_gold_exchange_on_return", 
        _gold_exchange.player_id_, _gold_exchange.exchange_id_, _gold_exchange.exchange_type_, _gold_exchange.amount_, _gold_exchange.unit_price_, _gold_exchange.state_ )
    --]]
    c_log( "%s exchange_id: %d", log_head, _gold_exchange.exchange_id_ )
end

function on_query_result_end()
    local log_head = "[gold_exchange_mng](on_query_result_end)"
    if g_init_state ~= INIT_STATE_RECVING_RESULT then
        c_err( "%s g_init_state: %d, not INIT_STATE_RECVING_RESULT", log_head, g_init_state )
        return
    end
    g_init_state = INIT_STATE_INITED
    c_log( "%s all gold_exchanges received, g_init_state: %d", log_head, g_init_state )
end

function on_bf_load_zone_info( _bf_id )
    local is_allow_all_server = true
    if _bf_id == 0 and is_allow_all_server then
        try_query_all_gold_exchanges()
    end
end

function on_bf_disconnect( _bf_id )
    if _bf_id == 0 and g_is_allow_all_bf then
        g_init_state = INIT_STATE_UNINIT
        c_log( "[gold_exchange_mng](on_bf_disconnect) center bf disconnect, g_init_state set to INIT_STATE_UNINIT" )
    end
end

function send_operate_fail_to_client( _player_id, _msg_id )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player:add_defined_text( _msg_id )
        player:add_operate_fail_gold_exchange()
    end
end

function send_gold_exchange_list_to_client( _player_id, _exchange_type, _page_index, _total_page, _gold_exchange_list )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player:add_onsell_gold_exchange( _exchange_type, _page_index, _total_page, _gold_exchange_list )
    end
end

function on_gold_exchange_inserted( _gold_exchange )
    local player_id   = _gold_exchange.player_id_
    local exchange_id = _gold_exchange.exchange_id_
    local player      = player_mng.get_player_by_player_id( player_id )
    if player then
        player:add_gold_exchange( _gold_exchange )
    end
    remote_call_bf( "gold_exchange_mng.add_gold_exchange", _gold_exchange )
    c_log( "[gold_exchange_mng](on_gold_exchange_inserted) player_id: %d, exchange_id: %d", player_id, exchange_id )
end

function consume_money_from_buyer( _buyer_player_id, _seller_server_id, _exchange_id, _exchange_type, _consume_amount )
    local log_head = "[gold_exchange_mng](consume_money_from_buyer)"

    local player = player_mng.get_player_by_player_id( _buyer_player_id )
    if not player then
        remote_call_bf( "gold_exchange_mng.unlock_gold_exchange", _seller_server_id, _exchange_id, true )
        c_log( "%s buyer offline, player_id: %d", log_head, _buyer_player_id )
        return
    end
    
    local consume_func
    if _exchange_type == share_const.EXCHANGE_TYPE_DIAMOND then
        consume_func = player_t.consume_non_bind_gold
    elseif _exchange_type == share_const.EXCHANGE_TYPE_GOLD then
        consume_func = player_t.consume_non_bind_diamond
    end

    if consume_func( player, _consume_amount, "[BUY_GOLD_EXCHANGE]" ) then
        local func_name = "db_gold_exchange.update_buyer_money"
        local diamond = player.non_bind_diamond_
        local gold = player.gold_
        dbclient.remote_call_db( func_name, _buyer_player_id, diamond, gold, _seller_server_id, _exchange_id )
    else
        local log_str = "%s consume fail, player_id: %d, exchange_id: [%d-%d], type: %d, consume_amount: %d"
        c_err( log_str, log_head, _buyer_player_id, _seller_server_id, _exchange_id, _exchange_type, _consume_amount )
        remote_call_bf( "gold_exchange_mng.unlock_gold_exchange", _seller_server_id, _exchange_id, true )
    end
end

function fetch_goods_for_buyer( _seller_server_id, _exchange_id, _buyer_player_id )
    local buyer_server_id = g_gamesvr:c_get_server_id()
    local func_name = "gold_exchange_mng.do_fetch_goods_for_buyer"
    remote_call_bf( func_name, buyer_server_id, _buyer_player_id, _seller_server_id, _exchange_id )
end

function save_gold_exchange_on_sell( _exchange_id, _seller_player_id, _buyer_server_id, _buyer_player_id )
    local func_name = "db_gold_exchange.save_gold_exchange_on_sell"
    dbclient.remote_call_db( func_name, _exchange_id, _seller_player_id, _buyer_server_id, _buyer_player_id )
end

function give_goods_to_buyer_from_db( _exchange_id, _seller_player_id, _buyer_server_id, _buyer_player_id )
    local log_head = "[gold_exchange_mng](give_goods_to_buyer_from_db)"
    gamesvr.operate_player_data( _seller_player_id, "player_t.on_gold_exchange_sold", nil, _exchange_id )
    local seller_server_id = g_gamesvr:c_get_server_id()
    remote_call_bf( "gold_exchange_mng.give_goods_to_buyer", seller_server_id, _exchange_id, _buyer_server_id, _buyer_player_id )
    c_log( "%s exchange_id: [%d-%d], buyer_server_id: %d, buyer_player_id: %d", log_head, seller_server_id, _exchange_id, _buyer_server_id, _buyer_player_id )
end

function do_give_goods_to_buyer( _buyer_player_id, _exchange_type, _amount )
    local log_head = "[gold_exchange_mng](do_give_diamond_to_buyer)"
    local player = player_mng.get_player_by_player_id( _buyer_player_id )
    if player then
        if _exchange_type == share_const.EXCHANGE_TYPE_DIAMOND then
            if player.non_bind_diamond_ + _amount > limit.INT_MAX then
                c_err( "%s diamond reach max, player_id: %d, amount: %d", log_head, _buyer_player_id, _amount )
            end
            player:add_bind_diamond( _amount, "BUY_GOLD_EXCHANGE" )
        elseif _exchange_type == share_const.EXCHANGE_TYPE_GOLD then
            if player.gold_ + _amount > limit.INT_MAX then
                c_err( "%s gold reach max, player_id: %d, amount: %d", log_head, _buyer_player_id, _amount )
            end
            player:add_non_bind_gold( _amount, "BUY_GOLD_EXCHANGE" )
        end
        player:on_gold_exchange_bought()
    else
        -- TODO add mail?
        c_log( "%s buyer offline, _player_id: %d", log_head, _buyer_player_id )
    end
end

function save_gold_exchange_on_withdraw( _player_id, _exchange_id, _exchange_type, _amount )
    c_log( "[gold_exchange_mng](save_gold_exchange_on_withdraw) exchange_id: %d, player_id: %d", _exchange_id, _player_id )
    dbclient.remote_call_db( "db_gold_exchange.save_gold_exchange_on_withdraw", _player_id, _exchange_id, _exchange_type, _amount )
end

function give_money_to_seller_on_withdraw( _player_id, _exchange_id, _exchange_type, _amount )
    local log_head = "[gold_exchange_mng](give_money_to_seller_on_withdraw)"
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        if _exchange_type == share_const.EXCHANGE_TYPE_DIAMOND then
            if player.non_bind_diamond_ + _amount > limit.INT_MAX then
                c_err( "%s diamond reach max, player_id: %d, exchange_id: %d, amount: %d", log_head, _player_id, _exchange_id, _amount )
            end
            player:add_non_bind_diamond( _amount, "WITHDRAW_GOLD_EXCHANGE" )
        elseif _exchange_type == share_const.EXCHANGE_TYPE_GOLD then
            if player.gold_ + _amount > limit.INT_MAX then
                c_err( "%s gold reach max, player_id: %d, exchange_id: %d, amount: %d", log_head, _player_id, _exchange_id, _amount )
            end
            player:add_non_bind_gold( _amount, "WITHDRAW_GOLD_EXCHANGE" )
        end
        player:on_gold_exchange_withdrew( _exchange_id, _exchange_type )
    else
        -- TODO add mail?
        c_log( "%s buyer offline, _player_id: %d", log_head, _buyer_player_id )
    end
    c_log( "%s player_id: %d, exchange_id: %d, type: %d, amount: %d", log_head, _player_id, _exchange_id, _exchange_type, _amount )
end

function save_gold_exchange_on_retrieve( _player_id, _exchange_id, _exchange_type, _amount )
    dbclient.remote_call_db( "db_gold_exchange.save_gold_exchange_on_retrieve", _player_id, _exchange_id, _exchange_type, _amount )
end

function give_money_to_seller_on_retrieve( _player_id, _exchange_id, _exchange_type, _amount )
    local log_head = "[gold_exchange_mng](give_money_to_seller_on_retrieve)"
    local sys_log = "RETRIEVE_GOLD_EXCHANGE"
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        if _exchange_type == share_const.EXCHANGE_TYPE_DIAMOND then
            local total_amount = _amount + share_const.EXCHANGE_DIAMOND_DEPOSIT
            if player.non_bind_diamond_ + total_amount > limit.INT_MAX then
                c_err( "%s diamond reach max, player_id: %d, exchange_id: %d", log_head, _player_id, _exchange_id )
            end
            player:add_non_bind_diamond( total_amount, sys_log )
        elseif _exchange_type == share_const.EXCHANGE_TYPE_GOLD then
            local total_amount = _amount + share_const.EXCHANGE_GOLD_DEPOSIT
            if player.gold_ + total_amount > limit.INT_MAX then
                c_err( "%s gold reach max, player_id: %d, exchange_id: %d", log_head, _player_id, _exchange_id )
            end
            player:add_non_bind_gold( total_amount, sys_log )
        end
        player:on_gold_exchange_retrieved( _exchange_id, _exchange_type )
    else
        -- TODO add mail?
        c_log( "%s buyer offline, _player_id: %d", log_head, _player_id )
    end
end

function save_gold_exchange_on_receive( _player_id, _exchange_id, _exchange_type, _amount, _unit_price )
    dbclient.remote_call_db( "db_gold_exchange.save_gold_exchange_on_receive", _player_id, _exchange_id, _exchange_type, _amount, _unit_price )
end

function give_money_to_seller_on_receive( _player_id, _exchange_id, _exchange_type, _amount, _unit_price )
    local sys_log = "RECEIVE_GOLD_EXCHANGE"
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        if _exchange_type == share_const.EXCHANGE_TYPE_DIAMOND then
            local total_amount = _amount / share_const.EXCHANGE_DIAMOND_UNIT_AMOUNT * _unit_price
            player:add_non_bind_gold( total_amount, sys_log )
            player:add_non_bind_diamond( share_const.EXCHANGE_DIAMOND_DEPOSIT, sys_log )
        elseif _exchange_type == share_const.EXCHANGE_TYPE_GOLD then
            local total_amount = _amount / share_const.EXCHANGE_GOLD_UNIT_AMOUNT * _unit_price
            player:add_bind_diamond( total_amount, sys_log )
            player:add_non_bind_gold( share_const.EXCHANGE_GOLD_DEPOSIT, sys_log )
        end
        player:on_gold_exchange_received( _exchange_id, _exchange_type )
    else
        -- TODO add mail?
        c_log( "[gold_exchange_mng](give_money_to_seller_on_receive) buyer offline, _player_id: %d", _player_id )
    end
end

function save_gold_exchange_on_timeout( _player_id, _exchange_id )
    dbclient.remote_call_db( "db_gold_exchange.save_gold_exchange_on_timeout", _player_id, _exchange_id )
end

function on_gold_exchange_timeout_from_db( _player_id, _exchange_id )
    gamesvr.operate_player_data( _player_id, "player_t.on_gold_exchange_timeout", nil, _exchange_id )
end

function give_money_to_seller_on_return( _player_id, _exchange_id, _exchange_type, _amount, _unit_price, _state )
    local sys_log = "RETURN_GOLD_EXCHANGE"
    local item_list = {}
    if _exchange_type == share_const.EXCHANGE_TYPE_DIAMOND then
        if _state == share_const.EXCHANGE_STATE_SOLD then
            local non_bind_diamond_item = item_t.new_by_system( share_const.NON_BIND_DIAMOND_ITEM_ID, share_const.EXCHANGE_DIAMOND_DEPOSIT, sys_log )
            local non_bind_gold_amount = _amount / share_const.EXCHANGE_DIAMOND_UNIT_AMOUNT * _unit_price
            local non_bind_gold_item = item_t.new_by_system( share_const.GOLD_ITEM_ID, non_bind_gold_amount, sys_log )
            tinsert( item_list, non_bind_diamond_item )
            tinsert( item_list, non_bind_gold_item )
        else
            local non_bind_diamond_amount = _amount + share_const.EXCHANGE_DIAMOND_DEPOSIT
            local non_bind_diamond_item = item_t.new_by_system( share_const.NON_BIND_DIAMOND_ITEM_ID, non_bind_diamond_amount, sys_log )
            tinsert( item_list, non_bind_diamond_item )
        end
    elseif _exchange_type == share_const.EXCHANGE_TYPE_GOLD then
        if _state == share_const.EXCHANGE_STATE_SOLD then
            local non_bind_gold_item = item_t.new_by_system( share_const.GOLD_ITEM_ID, share_const.EXCHANGE_GOLD_DEPOSIT, sys_log )
            local bind_diamond_amount = _amount / share_const.EXCHANGE_GOLD_UNIT_AMOUNT * _unit_price
            local bind_diamond_item = item_t.new_by_system( share_const.BIND_DIAMOND_ITEM_ID, bind_diamond_amount, sys_log )
            tinsert( item_list, non_bind_gold_item )
            tinsert( item_list, bind_diamond_item )
        else
            local non_bind_gold_amount = _amount + share_const.EXCHANGE_GOLD_DEPOSIT
            local non_bind_gold_item = item_t.new_by_system( share_const.GOLD_ITEM_ID, non_bind_gold_amount, sys_log )
            tinsert( item_list, non_bind_gold_item )
        end
    end
    mail_box_t.add_mail( _player_id, 20, item_list )
    c_log( "[gold_exchange_mng](give_money_to_seller_on_return) player_id: %d, exchange_id: %d, exchange_type: %d, amount: %d, unit_price: %d, state: %d", 
        _player_id, _exchange_id, _exchange_type, _amount, _unit_price, _state )
end
