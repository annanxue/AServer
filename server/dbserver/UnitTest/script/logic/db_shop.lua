module( "db_shop", package.seeall )

local sformat, tinsert, ostime, sgsub = string.format, table.insert, os.time, string.gsub

SELECT_ALL_SHOPS_SQL = [[
    SELECT 
        `shop_local_id`,
        `player_id`,
        `head_icon_id`,
        `shop_name`,
        `shop_type`,
        `description`,
        `gold`, 
        `bind_gold`,
        `bid_gold_yesterday`,
        `bid_gold_today`,
        `bid_gold_times`,
        unix_timestamp(last_maintain_time),
        `deal_history`,
        `items`,
        `next_item_sn_in_shop`
    FROM SHOP_TBL;
]]
SELECT_ALL_SHOPS_SQL_RESULT_TYPE = "iiisisiiiiiissd"

SELECT_PLAYER_SHOPS_SQL = [[
    SELECT
        `shop_local_id`,
        `shop_name`,
        `shop_type`
    FROM SHOP_TBL WHERE player_id = %d;
]]

SELECT_PLAYER_ID_SQL = [[ SELECT player_id FROM SHOP_TBL where shop_local_id = %d; ]]

INSERT_SHOP_SQL = [[
    INSERT INTO SHOP_TBL
    (
        `player_id`,
        `head_icon_id`,
        `shop_type`,
        `shop_name`,
        `description`,
        `gold`,
        `bind_gold`,
        `bid_gold_yesterday`,
        `bid_gold_today`,
        `bid_gold_times`,
        `last_maintain_time`,
        `deal_history`,
        `items`,
        `next_item_sn_in_shop`
    )
    VALUES
    (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        0,
        0,
        0,
        0,
        from_unixtime(?),
        '',
        '',
        1
    );
]]
INSERT_SHOP_SQL_PARAM_TYPE = "iiissii"

UPDATE_SHOP_ITEMS_SQL = [[
    UPDATE SHOP_TBL SET
        items = ?,
        next_item_sn_in_shop = ?
    WHERE shop_local_id = ?;
]]
UPDATE_SHOP_ITEMS_SQL_PARAM_TYPE = "sdi"

UPDATE_SHOP_SQL_ON_SELL = [[
    UPDATE SHOP_TBL SET
        gold = ?,
        bind_gold = ?,
        deal_history = ?,
        items = ?
    WHERE shop_local_id = ?;
]]
UPDATE_SHOP_SQL_ON_SELL_PARAM_TYPE = "iissi"

UPDATE_PLAYER_GOLD_SQL = [[ UPDATE PLAYER_TBL SET gold = %d WHERE player_id = %d; ]]

UPDATE_SHOP_ON_MAINTAIN_SQL = [[
    UPDATE SHOP_TBL SET
        gold = ?,
        bid_gold_yesterday = bid_gold_today,
        bid_gold_today = 0,
        bid_gold_times = 0,
        last_maintain_time = from_unixtime(?),
        deal_history = ?
    WHERE shop_local_id = ?;
]]
UPDATE_SHOP_ON_MAINTAIN_SQL_PARAM_TYPE = "iisi"

DELETE_SHOP_SQL = [[ DELETE FROM SHOP_TBL WHERE shop_local_id = %d; ]]

UPDATE_SHOP_GOLD_SQL = [[ UPDATE SHOP_TBL SET bind_gold = %d, gold = %d WHERE shop_local_id = %d; ]]

UPDATE_SHOP_BID_GOLD_SQL = [[ UPDATE SHOP_TBL SET bid_gold_today = %d, bid_gold_times = %d WHERE shop_local_id = %d; ]]

UPDATE_SHOP_NAME_DESC_SQL = [[
    UPDATE SHOP_TBL SET
        shop_name = ?,
        description = ?
    WHERE shop_local_id = ?;
]]
UPDATE_SHOP_NAME_DESC_SQL_PARAM_TYPE = "ssi"

UPDATE_SHOP_HEAD_ICON_SQL = [[ UPDATE SHOP_TBL SET head_icon_id = %d WHERE shop_local_id = %d; ]]

function query_all_shops()
    c_log( "[db_shop](query_all_shops) query begin" )

    local database = dbserver.g_db_character 

    if database:c_stmt_format_query( SELECT_ALL_SHOPS_SQL, "", SELECT_ALL_SHOPS_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_shop](query_all_shops) SELECT_ALL_SHOPS_SQL error" )
        return
    end

    dbserver.remote_call_game( "shop_mng.on_query_result_begin" )

    local rows = database:c_stmt_row_num()
    for i = 1, rows do
        if database:c_stmt_fetch() ~= 0 then
            c_err( "[db_shop](query_all_shops) fetch error" )
            return
        end

        local shop_local_id = database:c_stmt_get( "shop_local_id" )
        local player_id = database:c_stmt_get( "player_id" )
        local head_icon_id = database:c_stmt_get( "head_icon_id" )
        local shop_name = database:c_stmt_get( "shop_name" )
        local shop_type = database:c_stmt_get( "shop_type" )
        local shop_desc = database:c_stmt_get( "description" )
        local gold = database:c_stmt_get( "gold" )
        local bind_gold = database:c_stmt_get( "bind_gold" )
        local bid_gold_yesterday = database:c_stmt_get( "bid_gold_yesterday" )
        local bid_gold_today = database:c_stmt_get( "bid_gold_today" )
        local bid_gold_times = database:c_stmt_get( "bid_gold_times" )
        local last_maintain_time = database:c_stmt_get( "unix_timestamp(last_maintain_time)" )
        local deal_history = database:c_stmt_get( "deal_history" )
        local items = database:c_stmt_get( "items" )
        local next_item_sn_in_shop = database:c_stmt_get( "next_item_sn_in_shop" )

        if  not shop_local_id or 
            not player_id or 
            not head_icon_id or 
            not shop_name or 
            not shop_type or 
            not shop_desc or 
            not gold or 
            not bind_gold or 
            not bid_gold_yesterday or 
            not bid_gold_today or 
            not bid_gold_times or 
            not last_maintain_time or 
            not deal_history or
            not items or
            not next_item_sn_in_shop then
            c_err( "[db_shop](query_all_shops) get column error" )
            return
        end

        shop_name = sgsub( shop_name, "[\r\n]", "" )
        shop_desc = sgsub( shop_desc, "[\r\n]", "" )

        local shop =
        {
            shop_server_id_ = g_dbsvr:c_get_server_id(),
            shop_local_id_ = shop_local_id,
            player_id_ = player_id,
            head_icon_id_ = head_icon_id,
            shop_name_ = shop_name,
            shop_type_ = shop_type,
            shop_desc_ = shop_desc,
            gold_ = gold,
            bind_gold_ = bind_gold,
            bid_gold_yesterday_ = bid_gold_yesterday,
            bid_gold_today_ = bid_gold_today,
            bid_gold_times_ = bid_gold_times,
            last_maintain_time_ = last_maintain_time,
            deal_history_str_ = deal_history,
            items_str_ = items,
            next_item_sn_in_shop_ = next_item_sn_in_shop,
        }

        dbserver.remote_call_game( "shop_mng.on_shop_queried", shop )
    end

    dbserver.remote_call_game( "shop_mng.on_query_result_end" )

    c_log( "[db_shop](query_all_shops) query end, shop count: %d", rows )
end

function do_get_player_shops( _player_id )
    local database = dbserver.g_db_character 

    if database:c_query( sformat( SELECT_PLAYER_SHOPS_SQL, _player_id ) ) < 0 then
        c_err( "[db_shop](do_get_player_shops) SELECT_PLAYER_SHOPS_SQL error, player_id: %d", _player_id )
        return
    end

    local shops = {}

    local rows = database:c_row_num()
    for i = 1, rows do
        if database:c_fetch() ~= 0 then
            c_err( "[db_shop](do_get_player_shops) fetch error, player_id: %d", _player_id )
            return
        end

        local r1, shop_local_id = database:c_get_int32( "shop_local_id" )
        local r2, shop_name = database:c_get_str( "shop_name", 64 )
        local r3, shop_type = database:c_get_int32( "shop_type" )

        if  r1 < 0 or
            r2 < 0 or
            r3 < 0 then
            c_err( "[db_shop](do_get_player_shops) get column error, player_id: %d", _player_id )
            return
        end

        shop_name = sgsub( shop_name, "[\r\n]", "" )

        local shop =
        {
            shop_local_id_ = shop_local_id,
            shop_name_ = shop_name,
            shop_type_ = shop_type,
        }
        tinsert( shops, shop )
    end

    return shops
end

local function insert_shop_to_db( _player_gold, _shop )
    local database = dbserver.g_db_character 

    -- 为了避免金币数量因服务器宕机而被复制，对玩家金币的修改应立即存盘
    if database:c_modify( sformat( UPDATE_PLAYER_GOLD_SQL, _player_gold, _shop.player_id_ ) ) < 0 then
        c_err( "[db_shop](insert_shop_to_db) UPDATE_PLAYER_GOLD_SQL error, player_id: %d", _shop.player_id_ )
        return false
    end

    if database:c_stmt_format_modify( INSERT_SHOP_SQL
                                    , INSERT_SHOP_SQL_PARAM_TYPE
                                    , _shop.player_id_
                                    , _shop.head_icon_id_
                                    , _shop.shop_type_
                                    , _shop.shop_name_
                                    , _shop.shop_desc_
                                    , _shop.gold_
                                    , _shop.last_maintain_time_
                                    ) < 0 then
         c_err( "[db_shop](insert_shop_to_db) INSERT_SHOP_SQL error, player_id: %d", _shop.player_id_ )
         return false
     end

     _shop.shop_local_id_ = database:c_get_insert_id()

     return true
end

function create_shop( _params )
    local player_id = _params.player_id_

    local shop =
    {
        shop_server_id_ = g_dbsvr:c_get_server_id(),
        player_id_ = _params.player_id_,
        head_icon_id_ = _params.head_icon_id_,
        shop_type_ = _params.shop_type_,
        shop_name_ = _params.shop_name_,
        shop_desc_ = _params.shop_desc_,
        gold_ = share_const.SHOP_STARTUP_CAPITAL,
        bind_gold_ = 0,
        bid_gold_yesterday_ = 0,
        bid_gold_today_ = 0,
        bid_gold_times_ = 0,
        last_maintain_time_ = ostime(),
        reputations_past_ = {},
        reputations_today_ = {},
        deal_history_ = {},
        items_ = {},
        next_item_sn_in_shop_ = 1,
    }

    if not insert_shop_to_db( _params.player_gold_, shop ) then
        c_err( "[db_shop](create_shop) insert_shop_to_db sql error, player_id: %d", player_id )
        return
    end

    local player = db_login.g_player_cache:get( player_id )
    if player then
        player.gold_ = _params.player_gold_
        player.shops_ = do_get_player_shops( player_id )
    end

    dbserver.remote_call_game( "shop_mng.on_shop_created", shop )

    c_log( "[db_shop](create_shop) shop created, player_id: %d, shop_local_id: %d, shop_name: %s",
           player_id, shop.shop_local_id_, shop.shop_name_ )
end

function save_shop_items_by_ar( _ar )
    local shop_local_id = _ar:read_int()
    local items_str = _ar:read_string()
    local next_item_sn_in_shop = _ar:read_double()
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( UPDATE_SHOP_ITEMS_SQL
                                    , UPDATE_SHOP_ITEMS_SQL_PARAM_TYPE
                                    , items_str
                                    , next_item_sn_in_shop
                                    , shop_local_id
                                    ) < 0 then
        c_err( "[db_shop](save_shop_items_by_ar) UPDATE_SHOP_ITEMS_SQL error, shop_local_id: %d, next_item_sn_in_shop: %.0f"
             , shop_local_id, next_item_sn_in_shop
             )
        return
    end
    c_log( "[db_shop](save_shop_items_by_ar) shop_local_id: %d, next_item_sn_in_shop: %.0f", shop_local_id, next_item_sn_in_shop )
end

function save_shop_by_ar_on_sell( _shop_local_id, _ar )
    local gold = _ar:read_int()
    local bind_gold = _ar:read_int()
    local deal_history_str = _ar:read_string()
    local items_str = _ar:read_string()

    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( UPDATE_SHOP_SQL_ON_SELL
                                    , UPDATE_SHOP_SQL_ON_SELL_PARAM_TYPE
                                    , gold
                                    , bind_gold
                                    , deal_history_str
                                    , items_str
                                    , _shop_local_id
                                    ) < 0 then
        c_err( "[db_shop](save_shop_by_ar_on_sell) UPDATE_SHOP_SQL_ON_SELL error, shop_local_id: %d", _shop_local_id )
    end
end

function update_buyer_gold( _player_id, _gold, _shop_server_id, _shop_local_id, _item_sn_in_shop )
    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.gold_ = _gold
    end
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( UPDATE_PLAYER_GOLD_SQL, _gold, _player_id ) ) < 0 then
        c_err( "[db_shop](update_player_gold) UPDATE_PLAYER_GOLD_SQL error, player_id: %d", _player_id )
        return
    end
    dbserver.remote_call_game( "shop_mng.fetch_item_for_buyer", _shop_server_id, _shop_local_id, _item_sn_in_shop, _player_id )
    c_log( "[db_shop](update_buyer_gold) buyer gold updated, player_id: %d, gold: %d", _player_id, _gold )
end

function save_shop_on_maintain( _shop_local_id, _gold, _pay_tax_time, _deal_history_str )
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( UPDATE_SHOP_ON_MAINTAIN_SQL
                                    , UPDATE_SHOP_ON_MAINTAIN_SQL_PARAM_TYPE
                                    , _gold
                                    , _pay_tax_time
                                    , _deal_history_str
                                    , _shop_local_id
                                    ) < 0 then
        c_err( "[db_shop](save_shop_on_maintain) UPDATE_SHOP_ON_MAINTAIN_SQL error, shop_local_id: %d, pay_tax_time: %d"
             , _shop_local_id, _pay_tax_time
             )
        return
    end
    c_log( "[db_shop](save_shop_on_maintain) shop updated, shop_local_id: %d, pay_tax_time: %d", _shop_local_id, _pay_tax_time )
end

function delete_shop( _shop_local_id, _player_id )
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( DELETE_SHOP_SQL, _shop_local_id ) ) < 0 then
        c_err( "[db_shop](delete_shop) DELETE_SHOP_SQL error, shop_local_id: %d, player_id: %d", _shop_local_id, _player_id )
        return
    end

    local player_shops = do_get_player_shops( _player_id )
    if not player_shops then
        c_err( "[db_shop](delete_shops) do_get_player_shops fail, player_id: %d", _player_id )
        return
    end

    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.shops_ = player_shops
    end

    -- delete_shop执行前，玩家可能刚好拿了身上的旧shops表进游戏，因此不管shop_mng删除商店时玩家是否在线，
    -- dbserver将商店从数据库中删除后，都一定要反馈给game，让玩家更新自己身上的shops表。
    dbserver.remote_call_game( "shop_mng.update_player_shops", _player_id, player_shops )

    c_log( "[db_shop](delete_shop) shop deleted, shop_local_id: %d, player_id: %d", _shop_local_id, _player_id )
end

function update_shop_gold( _shop_local_id, _bind_gold, _gold )
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( UPDATE_SHOP_GOLD_SQL, _bind_gold, _gold, _shop_local_id ) ) < 0 then
        c_err( "[db_shop](update_shop_gold) UPDATE_SHOP_GOLD_SQL error, shop_local_id: %d, gold: %d", _shop_local_id, _gold )
        return
    end
    c_log( "[db_shop](update_shop_gold) shop gold updated, shop_local_id: %d, gold: %d", _shop_local_id, _gold )
end

function update_shop_bid_gold( _shop_local_id, _bid_gold_today, _bid_gold_times )
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( UPDATE_SHOP_BID_GOLD_SQL, _bid_gold_today, _bid_gold_times, _shop_local_id ) ) < 0 then
        c_err( "[db_shop](update_shop_bid_gold) UPDATE_SHOP_BID_GOLD_SQL error, shop_local_id: %d", _shop_local_id )
        return
    end
    c_log( "[db_shop](update_shop_bid_gold) shop bid gold updated, shop_local_id: %d", _shop_local_id )
end

function update_shop_name_desc( _shop_local_id, _player_id, _shop_name, _shop_desc )
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( UPDATE_SHOP_NAME_DESC_SQL
                                    , UPDATE_SHOP_NAME_DESC_SQL_PARAM_TYPE
                                    , _shop_name
                                    , _shop_desc
                                    , _shop_local_id
                                    ) < 0 then
       c_err( "[db_shop](update_shop_name_desc) UPDATE_SHOP_NAME_DESC_SQL error, shop_local_id: %d, player_id: %d", _shop_local_id, _player_id )
       return
    end
    c_log( "[db_shop](update_shop_name_desc) shop name and description updated, shop_local_id: %d, player_id: %d", _shop_local_id, _player_id )
end

function update_shop_head_icon( _shop_local_id, _head_icon_id )
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( UPDATE_SHOP_HEAD_ICON_SQL, _head_icon_id, _shop_local_id ) ) < 0 then
        c_err( "[db_shop](update_shop_head_icon) sql error, shop_local_id: %d, head_icon_id: %d", _shop_local_id, _head_icon_id )
        return
    end
    c_log( "[db_shop](update_shop_head_icon) shop_local_id: %d, head_icon_id: %d", _shop_local_id, _head_icon_id )
end
