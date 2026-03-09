module( "db_order", package.seeall )

local os_time, tinsert = os.time, table.insert

SELECT_ORDER_SQL = [[
    SELECT `order_id` from ORDER_TBL where order_id = ?;
]]
SELECT_ORDER_SQL_PARAM_TYPE = "L"
SELECT_ORDER_SQL_RESULT_TYPE = "L"

INSERT_ORDER_SQL = [[
    INSERT into ORDER_TBL ( `order_id`, `order_state`, `create_time`, `product_id`, `player_id`, `money`, `channel_label`, `channel_order_id`, `pay_channel`, `ext` ) 
    VALUES ( ?, ?, ?, ?, ?, 0, ?, "", "", "");
]]
INSERT_ORDER_SQL_PARAM_TYPE = "Liiiis"

CANCEL_ORDER_SQL = [[
    UPDATE ORDER_TBL set order_state = ? where order_id = ?;
]]
CANCEL_ORDER_SQL_PARAM_TYPE = "iL"

UPDATE_ORDER_SQL = [[
    UPDATE ORDER_TBL set order_state = ?, money = ? , channel_order_id = ? where order_id = ?;
]]
UPDATE_ORDER_SQL_PARAM_TYPE = "iisL"

DELETE_ORDER_SQL = [[
    DELETE from ORDER_TBL where order_id = ? limit 1;
]]
DELETE_ORDER_SQL_PARAM_TYPE = "L"

LOAD_ORDER_SQL = [[
    SELECT `order_id`, `order_state`, `create_time`, `product_id`, `player_id`, `pay_channel`, `ext`from ORDER_TBL
    where (`order_state` = ? and `create_time` > ?) or (`order_state` = ?);
]]
LOAD_ORDER_SQL_PARAM_TYPE = "iii"
LOAD_ORDER_SQL_RESULT_TYPE = "Liiiiss"

SAVE_RECHARGE_SQL = [[
    UPDATE PLAYER_TBL set non_bind_diamond = ? where player_id = ?;
]]
SAVE_RECHARGE_SQL_PARAM_TYPE = "ii"

SELECT_RECHARGE_TOTAL_SQL = [[
    SELECT `channel_label`, SUM(`money`) as `total_recharge` from ORDER_TBL group by channel_label;
]]

UPDATE_ORDER_INFO_SQL = [[ 
    UPDATE ORDER_TBL set pay_channel = ?, ext = ? where order_id = ?;
]]
UPDATE_ORDER_INFO_SQL_PARAM_TYPE = "ssL"

SELECT_ORDER_BY_PLAYER_ID_SQL = [[
    SELECT `order_id`, `order_state`, `create_time`, `product_id`, `player_id`, `pay_channel`, `ext`from ORDER_TBL
    where (`player_id` = ?);
]]
SELECT_ORDER_BY_PLAYER_ID_SQL_PARAM_TYPE = "i"
SELECT_ORDER_BY_PLAYER_ID_SQL_RESULT_TYPE = "Liiiiss"

local function check_create_order( _order_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ORDER_SQL, SELECT_ORDER_SQL_PARAM_TYPE, _order_id, SELECT_ORDER_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_order](check_create_order)failed to query order, order_id: %d", _order_id )
        return false
    end

    local rows = database:c_stmt_row_num()
    if rows > 0 then
        c_sells( "[db_order](check_create_order)order_id: %d repeat in db", _order_id )
        return false
    end

    return true
end

local function check_operate_order( _order_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ORDER_SQL, "L", _order_id, SELECT_ORDER_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_order](check_operate_order)failed to query order, order_id: %d", _order_id )
        return false
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        c_err( "[db_order](check_operate_order)order_id: %d not exsit in db", _order_id )
        return false
    end

    return true
end

function create_order( _order_id, _order_info, _channel_label )
    local time = c_cpu_ms()

    local player_id = _order_info.player_id_ 

    if not check_create_order( _order_id ) then
        dbserver.remote_call_game( "order_mng.on_db_create_order", false, _order_id, player_id )
        return false
    end

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( INSERT_ORDER_SQL, INSERT_ORDER_SQL_PARAM_TYPE, 
                                _order_id, 
                                _order_info.order_state_, 
                                _order_info.create_time_, 
                                _order_info.product_id_,
                                _order_info.player_id_ , 
                                _channel_label) < 0 then
        dbserver.remote_call_game( "order_mng.on_db_create_order", false, _order_id, player_id )
        c_err( "[db_order](create_order)failed to insert order, order_id: %d", _order_id )
        return false
    end

    dbserver.remote_call_game( "order_mng.on_db_create_order", true, _order_id, player_id )

    time = c_cpu_ms() - time
    c_prof( "[db_order](create_order)player_id: %d, order_id: %d, use time: %d", player_id, _order_id, time )

    return true
end

function cancel_order( _player_id, _order_id )
    local time = c_cpu_ms()

    if not check_operate_order( _order_id ) then
        return false
    end

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( CANCEL_ORDER_SQL, CANCEL_ORDER_SQL_PARAM_TYPE, order_mng.ORDER_STATE_PAY_CANCEL, _order_id ) < 0 then
        c_err( "[db_order](cancel_order)failed to cancel order, order_id: %d", _order_id )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_order](cancel_order)player_id: %d, order_id: %d, use time: %d", _player_id, _order_id, time )

    return true
end

function complete_order( _player_id, _order_id, _order_state, _money, _channel_order_id )
    local time = c_cpu_ms()

    if not check_operate_order( _order_id ) then
        return false
    end

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( UPDATE_ORDER_SQL, UPDATE_ORDER_SQL_PARAM_TYPE, _order_state, _money, _channel_order_id, _order_id ) < 0 then
        c_err( "[db_order](complete_order)failed to delete order, order_id: %d", _order_id )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_order](complete_order)player_id: %d, order_id: %d, order_state: %d, money: %d, channel_order_id: %s, use time: %d", 
        _player_id, _order_id, _order_state, _money, _channel_order_id, time )

    return true
end

function load_uncompleted_orders()
    local expire_days = order_mng.ORDER_EXPIRE_DAYS 
    local processing_state = order_mng.ORDER_STATE_PAY_PROCESSING
    local payed_state = order_mng.ORDER_STATE_PAY_SUCCESS 

    local valid_time = os_time() - expire_days * 24 * 3600

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( LOAD_ORDER_SQL, LOAD_ORDER_SQL_PARAM_TYPE, processing_state, valid_time, payed_state, LOAD_ORDER_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_order](load_uncompleted_orders)failed to load order, check sql code" )
        return {}
    end

    local uncompleted_orders = {}

    local rows = database:c_stmt_row_num()
    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local order_id = database:c_stmt_get( "order_id" )
            local order_state = database:c_stmt_get( "order_state" )
            local create_time = database:c_stmt_get( "create_time" )
            local product_id = database:c_stmt_get( "product_id" )
            local player_id = database:c_stmt_get( "player_id" )
            local pay_channel = database:c_stmt_get( "pay_channel" )
            local ext = database:c_stmt_get( "ext" )
            local order_info = order_mng.create_order_info( player_id, order_id, product_id, order_state, create_time, pay_channel, ext )
            tinsert( uncompleted_orders, order_info )
        else 
            c_err( "[db_order](load_uncompleted_orders)failed to fetch order" )
            return {}
        end
    end

    return uncompleted_orders
end

function save_recharge_given( _player_id, _non_bind_diamond )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( SAVE_RECHARGE_SQL, SAVE_RECHARGE_SQL_PARAM_TYPE, _non_bind_diamond, _player_id ) < 0 then
        c_err( "[db_order](save_recharge_given)failed to save recharge, player_id: %d", _player_id )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_order](save_recharge_given)player_id: %d, non_bind_diamond: %d, use time: %d", 
        _player_id, _non_bind_diamond, time ) 

    return true
end

function compensate_order( _player_id, _product_id, _order_id, _channel_label )
    if not check_create_order( _order_id ) then
        local database = dbserver.g_db_character
        if database:c_stmt_format_modify( DELETE_ORDER_SQL, DELETE_ORDER_SQL_PARAM_TYPE, _order_id ) < 0 then
            c_err( "[db_order](compensate_order)failed to delete order, order_id: %d", _order_id )
            return
        end
    end
    dbserver.remote_call_game( "order_mng.on_db_compensate_order", _player_id, _product_id, _order_id, _channel_label )
end

function query_recharge_total()
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_query( SELECT_RECHARGE_TOTAL_SQL ) < 0 then
        c_err( "[db_order](query_recharge_total)failed to query recharge total" )
        return
    end

    local recharges = {}
    local rows = database:c_row_num()
    for i = 1, rows, 1 do
        if database:c_fetch() == 0 then
            local channel_label, recharge_total = database:c_get_format( "si", "channel_label", "total_recharge" );
            tinsert( recharges, { channel_label, recharge_total } )
        end
    end

    dbserver.remote_call_game( "order_mng.on_db_query_recharge_total", recharges )

    time = c_cpu_ms() - time
    c_prof( "[db_order](query_recharge_total)use time: %d", time ) 
end

function update_order_info( _player_id, _order_id, _pay_channel, _ext )
    local time = c_cpu_ms()

    if not check_operate_order( _order_id ) then
        return false
    end

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( UPDATE_ORDER_INFO_SQL, UPDATE_ORDER_INFO_SQL_PARAM_TYPE, _pay_channel, _ext, _order_id ) < 0 then
        c_err( "[db_order](update_order_info)failed to update order player_id:%d order_id: %d", _player_id, _order_id )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_order](update_order_info)player_id: %d, order_id: %d pay_channel: %s ext: %s time:%d", 
        _player_id, _order_id, _pay_channel, _ext, time )

    return true
end

function get_player_orders(_player_id)
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ORDER_BY_PLAYER_ID_SQL, SELECT_ORDER_BY_PLAYER_ID_SQL_PARAM_TYPE,
        _player_id, SELECT_ORDER_BY_PLAYER_ID_SQL_RESULT_TYPE ) < 0 then

        c_err( "[db_order](get_player_orders)failed to load order, check sql code" )
        return {}
    end

    local orders = {}

    local rows = database:c_stmt_row_num()
    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local order_id = database:c_stmt_get( "order_id" )
            local order_state = database:c_stmt_get( "order_state" )
            local create_time = database:c_stmt_get( "create_time" )
            local product_id = database:c_stmt_get( "product_id" )
            local player_id = database:c_stmt_get( "player_id" )
            local pay_channel = database:c_stmt_get( "pay_channel" )
            local ext = database:c_stmt_get( "ext" )
            local order_info = order_mng.create_order_info( player_id, order_id, product_id, order_state, create_time, pay_channel, ext )
            tinsert( orders, order_info )
        else 
            c_err( "[db_order](get_player_orders)failed to fetch order" )
            return {}
        end
    end

    return orders
end

