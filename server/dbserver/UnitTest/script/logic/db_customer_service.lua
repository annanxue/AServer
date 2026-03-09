module( "db_customer_service", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert, utils.serialize_table
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_CUSTOMER_SERVICE_STR = "local ret={} return ret"

SELECT_CUSTOMER_SERVICE_SQL = [[
    SELECT
    `customer_service`
    from CUSTOMER_SERVICE_TBL where player_id = ?;
]]

SELECT_CUSTOMER_SERVICE_SQL_RESULT_TYPE = "s"

REPLACE_CUSTOMER_SERVICE_SQL = [[
    REPLACE INTO CUSTOMER_SERVICE_TBL ( `player_id`, `customer_service` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_CUSTOMER_SERVICE_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_customer_service( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CUSTOMER_SERVICE_SQL, "i", _player_id, SELECT_CUSTOMER_SERVICE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_customer_service](do_get_player_customer_service) c_stmt_format_query on SELECT_CUSTOMER_SERVICE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_CUSTOMER_SERVICE_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local customer_service_str = database:c_stmt_get( "customer_service" )
            return customer_service_str
        end
    end
    return nil
end

function do_save_player_customer_service( _player )
    local player_id = _player.player_id_
    local customer_service = _player.customer_service_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_CUSTOMER_SERVICE_SQL, REPLACE_CUSTOMER_SERVICE_SQL_PARAM_TYPE, player_id, customer_service )
    return ret
end

function on_customer_service_answer( _player_id, _idx, _answer, _state )
    local customer_service_str = do_get_player_customer_service(_player_id)
    local customer_service_tbl = loadstring(customer_service_str)()
    local dialog_list = customer_service_tbl.dialog_list_ or {}
    customer_service_tbl.dialog_list_ = dialog_list 

    local dialog = dialog_list[_idx]
    if not dialog then
        c_err("[db_customer_service](on_answer_player_question) player:%d idx:%d not exist", _player_id, _idx)
        return
    end

    dialog.answer_ = _answer
    dialog.state_ = _state 
    dialog.is_read_ = false 

    customer_service_str = ser_table(customer_service_tbl)
    local player = db_login.g_player_cache:get( _player_id )
    if player and  player.customer_service_ then
        player.customer_service_ = customer_service_str
    end

    do_save_player_customer_service({player_id_ = _player_id, customer_service_ = customer_service_str})   
    c_log("[db_customer_service] (on_answer_player_question) player:%d idx:%d answer success!", _player_id, _idx)
end
