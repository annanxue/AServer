module( "db_trade_statistic", package.seeall )

local sformat, tinsert, ser_table, strlen = string.format, table.insert, utils.serialize_table, string.len
--------------------------------------------------------------------------
--                  sql default
--------------------------------------------------------------------------
REPLACE_TRADE_STATISTIC_SQL = [[
    REPLACE INTO TRADE_STATISTIC_TBL (
    `item_id`,
    `data` 
    )
    VALUES(?,?) 
]]

REPLACE_TRADE_STATISTIC_PARAM_TYPE = "is"

SELECT_ALL_TRADE_STATISTIC = [[
    SELECT
    `item_id`, 
    `data`
    FROM TRADE_STATISTIC_TBL
]]
--------------------------------------------------------------------------
--                             外部接口           
--------------------------------------------------------------------------
function do_get_all_trade_statistic_data()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_TRADE_STATISTIC, "", "is" ) < 0 then
        c_err( "[db_trade_statistic](do_get_all_trade_statistic_data) c_stmt_format_query on SELECT_ALL_TRADE_STATISTIC failed")
        return nil
    end
    local rows = database:c_stmt_row_num()
    local result = {} 
    if rows == 0 then
        return {} 
    else 
        local count = 0
        for i=1, rows do 
            if database:c_stmt_fetch() == 0 then
                local item_id = database:c_stmt_get( "item_id" )
                local data = database:c_stmt_get( "data" )
                result[item_id] = data
                count = count + 1 

                if count >= 200 then
                    dbserver.split_remote_call_game("trade_statistic_mng.on_split_init_trade_statistic", result) 
                    reuslt = {}
                    count = 0
                end
            end
        end

        if count > 0 then
            dbserver.split_remote_call_game("trade_statistic_mng.on_split_init_trade_statistic", result) 
        end
    end

    dbserver.remote_call_game("trade_statistic_mng.on_init_trade_statistic") 
end

function do_save_trade_statistic( _item_list )
    for item_id, data in pairs(_item_list) do
        local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_TRADE_STATISTIC_SQL
        , REPLACE_TRADE_STATISTIC_PARAM_TYPE
        , item_id 
        , data )
        if ret < 0 then
            return
        end
    end
end

