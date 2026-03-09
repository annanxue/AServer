module( "db_auction", package.seeall )

local sformat, tinsert, ser_table, strlen, tremove = string.format, table.insert, utils.serialize_table, string.len, table.remove
--------------------------------------------------------------------------
--                  sql default
--------------------------------------------------------------------------
SELECT_AUCTION_SQL = [[
    SELECT
    `record_list`,
    `bid_list`
    FROM AUCTION_TBL where player_id = ?;
]]

SELECT_AUCTION_SQL_RESULT_TYPE = "ss"

REPLACE_AUCTION_SQL = [[
    REPLACE INTO AUCTION_TBL ( `player_id`, `record_list`, `bid_list`) VALUES 
    (
        ?,
        ?,
        ?
    )
]]

REPLACE_AUCTION_SQL_PARAM_TYPE = "iss"

SELECT_AUCTION_ITEM_SQL = [[
    SELECT 
    `item_id`,
    `item`
    FROM AUCTION_ITEM_TBL WHERE player_id = ?;
]]

SELECT_AUCTION_ITEM_SQL_RESULT_TYPE = "ls"

SELECT_ALL_AUCTION_ITEM_SQL = [[
   SELECT 
   `item_id`,
   `item`
   FROM AUCTION_ITEM_TBL;  
]]

SELECT_ALL_AUCTION_ITEM_SQL_RESULT_TYPE = "ls"

REPLACE_AUCTION_ITEM_SQL = [[
    REPLACE INTO AUCTION_ITEM_TBL (`item_id`, `player_id`, `item`) VALUES 
    (
        ?,
        ?,
        ?
    )
]]

REPLACE_AUCTION_ITEM_SQL_PARAM_TYPE = "lis"

DELETE_AUCTION_ITEM_SQL = [[
    DELETE FROM AUCTION_ITEM_TBL WHERE item_id=?;
]]

SELECT_MAX_ITEM_ID_SQL = [[
    SELECT max(`item_id`) as max_id FROM AUCTION_ITEM_TBL;
]]
--------------------------------------------------------------------------
--                            本地方法           
--------------------------------------------------------------------------
function get_player_auction_items(_player_id)
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_AUCTION_ITEM_SQL, "i", _player_id, SELECT_AUCTION_ITEM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_auction](get_player_auction_items) c_stmt_format_query on SELECT_AUCTION_ITEM_SQL failed, player id: %d", _player_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    local item_list = {}    
    for i=1, rows do 
        if database:c_stmt_fetch() == 0 then
            local auction_item = database:c_stmt_get( "item" )
            tinsert( item_list, auction_item )
        end
    end

    return item_list
end

local function get_max_auction_item_id()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MAX_ITEM_ID_SQL, "", "l" ) < 0 then
        c_err( "[db_auction](get_max_auction_item_id) c_stmt_format_query on SELECT_MAX_ITEM_ID_SQL failed" )
        return -1
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return -1 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local max_id = database:c_stmt_get( "max_id" )
            return max_id 
        end
    end
end
--------------------------------------------------------------------------
--                             外部接口           
--------------------------------------------------------------------------
function do_get_player_auction( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_AUCTION_SQL, "i", _player_id, SELECT_AUCTION_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_auction](do_get_player_auction) c_stmt_format_query on SELECT_AUCTION_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return {} 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local auction_data = {}
            local record_str = database:c_stmt_get( "record_list" )
            local bid_str = database:c_stmt_get( "bid_list" )
            auction_data.record_list_ = record_str
            auction_data.bid_list_ = bid_str
            auction_data.item_list_ = get_player_auction_items( _player_id ) 
            return auction_data 
        end
    end
    return nil
end

function do_save_player_auction( _player )
    local player_id = _player.player_id_
    local auction = _player.auction_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_AUCTION_SQL
                                                , REPLACE_AUCTION_SQL_PARAM_TYPE
                                                , player_id
                                                , auction.record_list_ 
                                                , auction.bid_list_)
    return ret
end

function do_get_all_auction_items()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_AUCTION_ITEM_SQL, "", SELECT_ALL_AUCTION_ITEM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_auction](do_get_all_auction_items) c_stmt_format_query on SELECT_ALL_AUCTION_ITEM_SQL failed" )
        return nil
    end

    local item_list = {}  
    local rows = database:c_stmt_row_num()
    for i=1, rows do 
        if database:c_stmt_fetch() == 0 then
            local item = database:c_stmt_get( "item" )
            tinsert(item_list, item)

            local count = #item_list
            if count >= 500 then
                dbserver.split_remote_call_game("auction_mng.split_init_auction",  item_list) 
                item_list = {}
            end
        end
    end

    if #item_list > 0 then
        dbserver.split_remote_call_game("auction_mng.split_init_auction",  item_list) 
    end

    local max_item_id = get_max_auction_item_id() 
    dbserver.split_remote_call_game("auction_mng.init_auction",  max_item_id)
end

function do_del_auction_item(_player_id, _item_id)
    local db_item_id = auction_item_t.compute_db_auction_item_id( _item_id ) 
    
    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( DELETE_AUCTION_ITEM_SQL, "l", db_item_id) < 0 then
        c_err( "[db_auction](do_del_auction_item) DELETE_AUCTION_ITEM_SQL error, item_id: %d", db_item_id)
        return
    end

    c_log( "[db_auction](do_del_auction_item) player_id:%d item id: %s, db item_id: %d", _player_id, _item_id, db_item_id)
end

function do_save_auction_item(_player_id, _item_list)
    local database = dbserver.g_db_character 
    serialize_auction_item_to_peer = auction_item_t.serialize_auction_item_to_peer
    local compute_db_auction_item_id = auction_item_t.compute_db_auction_item_id 
    for _, auction_item in pairs(_item_list) do
        local item_str = serialize_auction_item_to_peer(auction_item)
        local item_id = auction_item.id_
        local db_item_id = compute_db_auction_item_id( item_id ) 
        local real_item_id = auction_item.real_item_.item_id_
        if database:c_stmt_format_modify( REPLACE_AUCTION_ITEM_SQL, REPLACE_AUCTION_ITEM_SQL_PARAM_TYPE, db_item_id, _player_id, item_str) < 0 then
            c_err( "[db_auction](do_save_auction_item) REPLACE_AUCTION_ITEM_SQL error, player_id:%d item id: %s db_item_id:%d real_item_id:%d", _player_id, item_id, db_item_id, real_item_id)
            return
        end
    end

    c_log( "[db_auction](do_save_auction_item) player_id:%d", _player_id)
end

function do_save_auction_record(_player_id, _record)
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_AUCTION_SQL, "i", _player_id, SELECT_AUCTION_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_auction](do_save_auction_record) c_stmt_format_query on SELECT_AUCTION_SQL failed, player id: %d", _player_id )
        return
    end
    local rows = database:c_stmt_row_num()
    local record_list = nil 
    local auction_str = "local ret ={} return ret" 
    if rows == 0 then
        record_list = {} 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local record_str = database:c_stmt_get( "record_list" )
            local bid_str = database:c_stmt_get( "bid_list" )
            record_list = loadstring(record_str)()
        end
    end
    
    local record_count = #record_list
    if record_count >= 100 then
        tremove(record_list, 1) 
    end
    tinsert(record_list, _record)
    local record_str = ser_table(record_list)
    if not dbserver.g_db_character:c_stmt_format_modify( REPLACE_AUCTION_SQL, REPLACE_AUCTION_SQL_PARAM_TYPE, _player_id, record_str, bid_str) then
        c_err( "[db_auction](do_save_auction_record) c_stmt_format_modify on REPLACE_AUCTION_SQL failed, player id: %d", _player_id )
        return
    end

    local player = db_login.g_player_cache:get( _player_id )
    if player and  player.auction_ then
        local auction_tbl = player.auction_ 
        auction_tbl.record_list_ = record_str 
    end

    c_log("[db_auction](do_save_auction_record) save player:%d auction record", _player_id)
end
