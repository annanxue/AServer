module( "db_gold_exchange", package.seeall )

local sformat, ostime = string.format, os.time

SELECT_ALL_SQL = [[
    SELECT 
        `exchange_id`,
        `player_id`,
        `exchange_type`,
        `state`,
        `amount`,
        `unit_price`,
        unix_timestamp(end_time) as end_time
    FROM GOLD_EXCHANGE_TBL
]]

SELECT_ALL_RESULT_TYPE = "iiiiiii"

SELECT_FOR_PLAYER_SQL = [[
    SELECT
        `exchange_id`,
        `player_id`,
        `exchange_type`,
        `state`,
        `amount`,
        `unit_price`,
        unix_timestamp(end_time) as end_time
    FROM GOLD_EXCHANGE_TBL
    WHERE player_id = ?;
]]

SELECT_FOR_PLAYER_PARAM_TYPE = "i"

SELECT_FOR_PLAYER_RESULT_TYPE = "iiiiiii"

INSERT_SQL = [[
    INSERT INTO GOLD_EXCHANGE_TBL
    (
        `player_id`,
        `exchange_type`,
        `state`,
        `amount`,
        `unit_price`,
        `end_time`
    )
    VALUES
    (
        ?,
        ?,
        ?,
        ?,
        ?,
        from_unixtime(?)
    );
]]

INSERT_PARAM_TYPE = "iiiiii"

UPDATE_MONEY_SQL = [[ UPDATE PLAYER_TBL SET non_bind_diamond = ?, gold = ? WHERE player_id = ?; ]]

UPDATE_MONEY_PARAM_TYPE = "iii"

UPDATE_STATE_SQL = [[
    UPDATE GOLD_EXCHANGE_TBL SET
        state = ?
    WHERE exchange_id = ? and state = ?
]]

UPDATE_STATE_PARAM_TYPE = "iii"

DELETE_SQL = [[
    DELETE FROM GOLD_EXCHANGE_TBL
    where exchange_id = ? and state = ?
]]

DELETE_PARAM_TYPE = "ii"

function query_all_gold_exchanges()
    c_log( "[db_gold_exchange](query_all_gold_exchanges) query begin" )

    local database = dbserver.g_db_character 
    if database:c_stmt_format_query( SELECT_ALL_SQL, "", SELECT_ALL_RESULT_TYPE ) < 0 then
        c_err( "[db_gold_exchange](query_all_gold_exchanges) SELECT_ALL_SQL error" )
        return
    end

    dbserver.remote_call_game( "gold_exchange_mng.on_query_result_begin" )
    local server_id = g_dbsvr:c_get_server_id()
    local rows = database:c_stmt_row_num()
    for i = 1, rows do
        if database:c_stmt_fetch() ~= 0 then
            c_err( "[db_gold_exchange](query_all_gold_exchanges) fetch error" )
            return
        end

        local exchange_id   = database:c_stmt_get( "exchange_id" )
        local player_id     = database:c_stmt_get( "player_id" )
        local exchange_type = database:c_stmt_get( "exchange_type" )
        local state         = database:c_stmt_get( "state" )
        local amount        = database:c_stmt_get( "amount" )
        local unit_price    = database:c_stmt_get( "unit_price" )
        local end_time      = database:c_stmt_get( "end_time" )

        if not exchange_id or 
           not player_id or 
           not exchange_type or 
           not state or 
           not amount or 
           not unit_price or
           not end_time then
            c_err( "[db_gold_exchange](query_all_gold_exchanges) get column error" )
            return
        end

        local gold_exchange =
        {
            server_id_     = server_id, 
            exchange_id_   = exchange_id,
            player_id_     = player_id,
            exchange_type_ = exchange_type,
            state_         = state,
            amount_        = amount,
            unit_price_    = unit_price,
            end_time_      = end_time,
        }

        dbserver.remote_call_game( "gold_exchange_mng.on_gold_exchange_queried", gold_exchange )
    end

    dbserver.remote_call_game( "gold_exchange_mng.on_query_result_end" )
    c_log( "[db_gold_exchange](query_all_gold_exchanges) query end, gold_exchange count: %d", rows )
end

function do_get_player_gold_exchanges( _player_id )
    local database = dbserver.g_db_character 
    if database:c_stmt_format_query( SELECT_FOR_PLAYER_SQL, SELECT_FOR_PLAYER_PARAM_TYPE, _player_id, SELECT_FOR_PLAYER_RESULT_TYPE ) < 0 then
        c_err( "[db_gold_exchange](do_get_player_gold_exchanges) SELECT_FOR_PLAYER_SQL error, player_id: %d", _player_id )
        return
    end

    local gold_exchanges_str = ""
    local rows = database:c_stmt_row_num()
    for i = 1, rows do
        if database:c_stmt_fetch() ~= 0 then
            c_err( "[db_gold_exchange](do_get_player_gold_exchanges) fetch error, player_id: %d", _player_id )
            return
        end

        local exchange_id   = database:c_stmt_get( "exchange_id" )
        local player_id     = database:c_stmt_get( "player_id" )
        local exchange_type = database:c_stmt_get( "exchange_type" )
        local state         = database:c_stmt_get( "state" )
        local amount        = database:c_stmt_get( "amount" )
        local unit_price    = database:c_stmt_get( "unit_price" )
        local end_time      = database:c_stmt_get( "end_time" )

        if not exchange_id or 
           not player_id or 
           not exchange_type or 
           not state or 
           not amount or 
           not unit_price or
           not end_time then
            c_err( "[db_gold_exchange](do_get_player_gold_exchanges) get column error, player_id: %d", _player_id )
            return
        end
        local str = sformat( "%d,%d,%d,%d,%d,%d,%d,",  exchange_id, player_id, exchange_type, state, amount, unit_price, end_time )
        gold_exchanges_str = gold_exchanges_str .. str
    end

    return gold_exchanges_str
end

function insert_gold_exchange( _params )
    local log_head = "[db_gold_exchange](insert_gold_exchange)"
    local server_id      = g_dbsvr:c_get_server_id()
    local player_id      = _params.player_id_
    local exchange_type  = _params.exchange_type_
    local state          = share_const.EXCHANGE_STATE_ONSELL
    local amount         = _params.amount_
    local unit_price     = _params.unit_price_
    local end_time       = ostime() + share_const.EXCHANGE_ONSELL_SECONDS
    local player_diamond = _params.player_non_bind_diamond_
    local player_gold    = _params.player_gold_
    c_log( "%s inserting, player_id: %d, type: %d, amount: %d, price: %d", log_head, player_id, exchange_type, amount, unit_price )

    -- save diamond and gold
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( UPDATE_MONEY_SQL, UPDATE_MONEY_PARAM_TYPE, player_diamond, player_gold, player_id ) < 0 then
        c_err( "%s UPDATE_MONEY_SQL error, player_id: %d", log_head, player_id )
        return
    end

    -- insert gold_exchange
    if database:c_stmt_format_modify( INSERT_SQL, INSERT_PARAM_TYPE, player_id, exchange_type, state, amount, unit_price, end_time ) < 0 then
        c_err( "%s INSERT_SQL error, player_id: %d", log_head, player_id )
        return
    end
    local exchange_id = database:c_get_insert_id()

    -- update player cache
    local player = db_login.g_player_cache:get( player_id )
    if player then
        player.gold_ = player_gold
        player.non_bind_diamond_ = player_diamond
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( player_id )
    end

    -- notify gameserver
    local gold_exchange =
    {
        exchange_id_   = exchange_id,
        server_id_     = server_id,
        player_id_     = player_id,
        exchange_type_ = exchange_type,
        state_         = state,
        amount_        = amount,
        unit_price_    = unit_price,
        end_time_      = end_time,
    }
    dbserver.remote_call_game( "gold_exchange_mng.on_gold_exchange_inserted", gold_exchange )
    local log_str = "%s inserted, player_id: %d, exchange_id: %d, type: %d, amount: %d, price: %d"
    c_log( log_str, log_head, player_id, exchange_id, exchange_type, amount, unit_price )
end

function update_buyer_money( _buyer_player_id, _diamond, _gold, _seller_server_id, _exchange_id )
    local log_head = "[db_gold_exchange](update_buyer_money)"

    -- update player cache
    local player = db_login.g_player_cache:get( _buyer_player_id )
    if player then
        player.non_bind_diamond_ = _diamond
        player.gold_ = _gold
    end

    -- update db
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( UPDATE_MONEY_SQL, UPDATE_MONEY_PARAM_TYPE, _diamond, _gold, _buyer_player_id ) < 0 then
        c_err( "%s UPDATE_MONEY_SQL error, player_id: %d", log_head, _buyer_player_id )
        return
    end

    -- notify gameserver
    dbserver.remote_call_game( "gold_exchange_mng.fetch_goods_for_buyer", _seller_server_id, _exchange_id, _buyer_player_id )
    c_log( "%s updated, buyer_player_id: %d, diamond: %d, gold: %d", log_head, _buyer_player_id, _diamond, _gold )
end

function save_gold_exchange_on_timeout( _player_id, _exchange_id )
    local log_head = "[db_gold_exchange](save_gold_exchange_on_timeout)"

    -- update state
    local database = dbserver.g_db_character 
    local new_state = share_const.EXCHANGE_STATE_TIMEOUT
    local old_state = share_const.EXCHANGE_STATE_ONSELL
    if database:c_stmt_format_modify( UPDATE_STATE_SQL, UPDATE_STATE_PARAM_TYPE, new_state, _exchange_id, old_state ) < 0 then
        c_err( "%s UPDATE_STATE_SQL error, exchange_id: %d", log_head, _exchange_id )
        return
    end
    
    -- update player cache
    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( _player_id )
    end

    -- notify gameserver
    local func_name = "gold_exchange_mng.on_gold_exchange_timeout_from_db"
    dbserver.remote_call_game( func_name, _player_id, _exchange_id )
    c_log( "%s gold exchange state updated, exchange_id: %d", log_head, _exchange_id )
end

function save_gold_exchange_on_sell( _exchange_id, _seller_player_id, _buyer_server_id, _buyer_player_id ) 
    local log_head = "[db_gold_exchange](save_gold_exchange_on_sell)"

    -- update state
    local database = dbserver.g_db_character 
    local new_state = share_const.EXCHANGE_STATE_SOLD
    local old_state = share_const.EXCHANGE_STATE_ONSELL
    if database:c_stmt_format_modify( UPDATE_STATE_SQL, UPDATE_STATE_PARAM_TYPE, new_state, _exchange_id, old_state ) < 0 then
        c_err( "%s UPDATE_STATE_SQL error, exchange_id: %d", log_head, _exchange_id )
        return
    end

    -- upate player cache
    local player = db_login.g_player_cache:get( _seller_player_id )
    if player then
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( _seller_player_id )
    end
    
    -- notify gameserver
    local func_name = "gold_exchange_mng.give_goods_to_buyer_from_db"
    dbserver.remote_call_game( func_name, _exchange_id, _seller_player_id, _buyer_server_id, _buyer_player_id )
    c_log( "%s state updated, exchange_id: %d", log_head, _exchange_id )
end

function save_gold_exchange_on_withdraw( _player_id, _exchange_id, _exchange_type, _amount )
    local log_head = "[db_gold_exchange](save_gold_exchange_on_withdraw)"

    -- delete from db
    local database = dbserver.g_db_character 
    local state = share_const.EXCHANGE_STATE_ONSELL
    if database:c_stmt_format_modify( DELETE_SQL, DELETE_PARAM_TYPE, _exchange_id, state ) < 0 then
        c_err( "%s DELETE_SQL error, exchange_id: %d", log_head, _exchange_id )
        return
    end

    -- update player cache
    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( _player_id )
    end

    -- notify gameserver
    local func_name = "gold_exchange_mng.give_money_to_seller_on_withdraw"
    dbserver.remote_call_game( func_name, _player_id, _exchange_id, _exchange_type, _amount )
    c_log( "%s withdrew, exchange_id: %d", log_head, _exchange_id )
end

function save_gold_exchange_on_retrieve( _player_id, _exchange_id, _exchange_type, _amount )
    local log_head = "[db_gold_exchange](save_gold_exchange_on_retrieve)"

    -- delete from db
    local database = dbserver.g_db_character 
    local state = share_const.EXCHANGE_STATE_TIMEOUT
    if database:c_stmt_format_modify( DELETE_SQL, DELETE_PARAM_TYPE, _exchange_id, state ) < 0 then
        c_err( "%s DELETE_SQL error, exchange_id: %d", log_head, _exchange_id )
        return
    end   

    -- update player cache
    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( _player_id )
    end

    -- notify gameserver
    local func_name = "gold_exchange_mng.give_money_to_seller_on_retrieve"
    dbserver.remote_call_game( func_name, _player_id, _exchange_id, _exchange_type, _amount )
    c_log( "%s gold exchange deleted, exchange_id: %d", log_head, _exchange_id )
end

function save_gold_exchange_on_receive( _player_id, _exchange_id, _exchange_type, _amount, _unit_price )
    local log_head = "[db_gold_exchange](save_gold_exchange_on_receive)"

    -- delete from db
    local database = dbserver.g_db_character 
    local state = share_const.EXCHANGE_STATE_SOLD
    if database:c_stmt_format_modify( DELETE_SQL, DELETE_PARAM_TYPE, _exchange_id, state ) < 0 then
        c_err( "%s DELETE_SQL error, exchange_id: %d", log_head, _exchange_id )
        return
    end

    -- update player cache
    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( _player_id )
    end

    -- notify gameserver
    local func_name = "gold_exchange_mng.give_money_to_seller_on_receive"
    dbserver.remote_call_game( func_name, _player_id, _exchange_id, _exchange_type, _amount, _unit_price )
    c_log( "%s gold exchange deleted, exchange_id: %d", log_head, _exchange_id )
end

function save_gold_exchange_on_return( _player_id, _exchange_id, _exchange_type, _amount, _unit_price, _state )
    local log_head = "[db_gold_exchange](delete_gold_exchange)"

    -- delete from db
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( DELETE_SQL, DELETE_PARAM_TYPE, _exchange_id, _state ) < 0 then
        c_err( "%s DELETE_SQL error, exchange_id: %d", log_head, _exchange_id )
        return
    end

    -- update player cache
    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.gold_exchanges_str_ = do_get_player_gold_exchanges( _player_id )
    end

    -- notify gameserver
    local func_name = "gold_exchange_mng.give_money_to_seller_on_return"
    dbserver.remote_call_game( func_name, _player_id, _exchange_id, _exchange_type, _amount, _unit_price, _state )
    c_log( "%s gold exchange deleted, exchange_id: %d", log_head, _exchange_id )
end
