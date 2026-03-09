module( "db_trade", package.seeall )

local sformat, tinsert, ser_table, strlen, tremove = string.format, table.insert, utils.serialize_table, string.len, table.remove
--------------------------------------------------------------------------
--                  sql default
--------------------------------------------------------------------------
SELECT_TRADE_SQL = [[
    SELECT
    `record_list`,
    `favorite_list`
    FROM TRADE_TBL where player_id = ?;
]]

SELECT_TRADE_SQL_RESULT_TYPE = "ss"

REPLACE_TRADE_SQL = [[
    REPLACE INTO TRADE_TBL ( `player_id`, `record_list`, `favorite_list`) VALUES 
    (
        ?,
        ?,
        ?
    )
]]

REPLACE_TRADE_SQL_PARAM_TYPE = "iss"

SELECT_TRADE_ITEM_SQL = [[
    SELECT 
    `item_id`,
    `item`
    FROM TRADE_ITEM_TBL WHERE player_id = ?;
]]

SELECT_TRADE_ITEM_SQL_RESULT_TYPE = "ls"

SELECT_ALL_TRADE_ITEM_SQL = [[
   SELECT 
   `item_id`,
   `item`
   FROM TRADE_ITEM_TBL;  
]]

SELECT_ALL_TRADE_ITEM_SQL_RESULT_TYPE = "ls"

REPLACE_TRADE_ITEM_SQL = [[
    REPLACE INTO TRADE_ITEM_TBL (`item_id`, `player_id`, `item`) VALUES 
    (
        ?,
        ?,
        ?
    )
]]

REPLACE_TRADE_ITEM_SQL_PARAM_TYPE = "lis"

DELETE_TRADE_ITEM_SQL = [[
    DELETE FROM TRADE_ITEM_TBL WHERE item_id=?;
]]

SELECT_MAX_ITEM_ID_SQL = [[
    SELECT max(`item_id`) as max_id FROM TRADE_ITEM_TBL;
]]
--------------------------------------------------------------------------
--                            本地方法           
--------------------------------------------------------------------------
local function get_player_trade_items(_player_id)
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TRADE_ITEM_SQL, "i", _player_id, SELECT_TRADE_ITEM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_trade](get_player_trade_items) c_stmt_format_query on SELECT_TRADE_ITEM_SQL failed, player id: %d", _player_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    local item_list = {}    
    for i=1, rows do 
        if database:c_stmt_fetch() == 0 then
            local trade_item = database:c_stmt_get( "item" )
            tinsert( item_list, trade_item )
        end
    end

    return item_list
end

local function get_max_trade_item_id()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MAX_ITEM_ID_SQL, "", "l" ) < 0 then
        c_err( "[db_trade](get_max_trade_item_id) c_stmt_format_query on SELECT_MAX_ITEM_ID_SQL failed" )
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
function do_get_player_trade( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TRADE_SQL, "i", _player_id, SELECT_TRADE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_trade](do_get_player_trade) c_stmt_format_query on SELECT_TRADE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return {} 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local trade_data = {}
            local record_str = database:c_stmt_get( "record_list" )
            local favorite_str = database:c_stmt_get( "favorite_list" )
            trade_data.record_list_ = record_str
            trade_data.favorite_list_ = favorite_str
            trade_data.item_list_ = get_player_trade_items( _player_id ) 
            return trade_data 
        end
    end
    return nil
end

function do_save_player_trade( _player )
    local player_id = _player.player_id_
    local trade = _player.trade_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_TRADE_SQL
                                                , REPLACE_TRADE_SQL_PARAM_TYPE
                                                , player_id
                                                , trade.record_list_ 
                                                , trade.favorite_list_)
    return ret
end

function do_get_all_trade_items()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_TRADE_ITEM_SQL, "", SELECT_ALL_TRADE_ITEM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_trade](do_get_all_trade_items) c_stmt_format_query on SELECT_ALL_TRADE_ITEM_SQL failed" )
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
                dbserver.split_remote_call_game("trade_mng.split_init_trade",  item_list) 
                item_list = {}
            end
        end
    end

    if #item_list > 0 then
        dbserver.split_remote_call_game("trade_mng.split_init_trade",  item_list) 
    end

    local max_item_id = get_max_trade_item_id() 
    dbserver.split_remote_call_game("trade_mng.init_trade",  max_item_id)
end

function do_del_trade_item(_player_id, _item_id)
    local db_item_id = trade_item_t.compute_db_trade_item_id( _item_id ) 

    local database = dbserver.g_db_character 
    if database:c_stmt_format_modify( DELETE_TRADE_ITEM_SQL, "l", db_item_id) < 0 then
        c_err( "[db_trade](do_del_trade_item) DELETE_TRADE_ITEM_SQL error, item_id: %d", db_item_id)
        return
    end

    c_log( "[db_trade](do_del_trade_item) player_id:%d item id: %s, db item_id: %d", _player_id, _item_id, db_item_id)
end

function do_save_trade_item(_player_id, _item_list)
    local database = dbserver.g_db_character 
    serialize_trade_item_to_peer = trade_item_t.serialize_trade_item_to_peer   
    local compute_db_trade_item_id = trade_item_t.compute_db_trade_item_id 
    for _, trade_item in pairs(_item_list) do
        local item_str = serialize_trade_item_to_peer(trade_item)
        local item_id = trade_item.id_
        local db_item_id = compute_db_trade_item_id( item_id ) 
        local real_item_id = trade_item.real_item_.item_id_
        if database:c_stmt_format_modify( REPLACE_TRADE_ITEM_SQL, REPLACE_TRADE_ITEM_SQL_PARAM_TYPE, db_item_id, _player_id, item_str) < 0 then
            c_err( "[db_trade](do_save_trade_item) REPLACE_TRADE_ITEM_SQL error, player_id:%d item_id:%s db_item_id: %d, real_item_id:%d", _player_id, item_id, db_item_id, real_item_id)
            return
        end
    end
end

function do_save_trade_record(_player_id, _record)
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TRADE_SQL, "i", _player_id, SELECT_TRADE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_trade](do_save_trade_record) c_stmt_format_query on SELECT_TRADE_SQL failed, player id: %d", _player_id )
        return
    end
    local rows = database:c_stmt_row_num()
    local record_list = nil 
    local favorite_str = "local ret ={} return ret" 
    if rows == 0 then
        record_list = {} 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local record_str = database:c_stmt_get( "record_list" )
            local favorite_str = database:c_stmt_get( "favorite_list" )
            record_list = loadstring(record_str)()
        end
    end
    
    local record_count = #record_list
    if record_count >= 100 then
        tremove(record_list, 1) 
    end
    tinsert(record_list, _record)
    local record_str = ser_table(record_list)
    if not dbserver.g_db_character:c_stmt_format_modify( REPLACE_TRADE_SQL, REPLACE_TRADE_SQL_PARAM_TYPE, _player_id, record_str, favorite_str) then
        c_err( "[db_trade](do_save_trade_record) c_stmt_format_modify on REPLACE_TRADE_SQL failed, player id: %d", _player_id )
        return
    end

    local player = db_login.g_player_cache:get( _player_id )
    if player and  player.trade_ then
        local trade_tbl = player.trade_ 
        trade_tbl.record_list_ = record_str 
    end

    c_log("[db_trade](do_save_trade_record) save player:%d trade record", _player_id)
end
