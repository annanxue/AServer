module( "db_system_global_data", package.seeall )

local sformat = string.format

SELECT_SYSTEM_GLOBAL_DATA_SQL = [[
    SELECT
    `data_str`
    from SYSTEM_GLOBAL_DATA_TBL where data_id = ?; 
]]

SELECT_SYSTEM_GLOBAL_DATA_SQL_RES_TYPE = "s"

REPLACE_SYSTEM_GLOBAL_DATA_SQL = [[
    REPLACE INTO SYSTEM_GLOBAL_DATA_TBL
    ( `data_id`, `data_str` ) VALUES
    (
       ?,
       ?
    )
]]

REPLACE_SYSTEM_GLOBAL_DATA_PARAM_TYPE = "is"

DELETE_SYSTEM_GLOBAL_DATA_SQL = [[
    DELETE FROM SYSTEM_GLOBAL_DATA_TBL WHERE data_id = ?;
]]

DELETE_SYSTEM_GLOBAL_DATA_SQL_PARAM_TYPE = "i"

local LOG_HEAD = "[db_system_global_data]"

local function get_data_id( _system_id, _sub_id )
    return _system_id * share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID + _sub_id
end

local function do_get_system_global_data( _system_id, _sub_id )
    local log_head = sformat( "%s(do_get_system_global_data)", LOG_HEAD )
    if _sub_id >= share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID then
        c_err( "%s sub_id: %d is exceed max value: %d, system_id: %d", log_head, _sub_id, share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID, _system_id )
        return nil
    end

    local data_id = get_data_id( _system_id, _sub_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SYSTEM_GLOBAL_DATA_SQL, "i", data_id, SELECT_SYSTEM_GLOBAL_DATA_SQL_RES_TYPE ) < 0 then
        c_err( "%s c_stmt_format_query on SELECT_SYSTEM_GLOBAL_DATA_SQL failed, system_id: %d, sub_id: %d", log_head, _system_id, _sub_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return "{}"
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local data_str = database:c_stmt_get( "data_str" )
            return data_str
        end
    end
end

function get_system_global_data( _system_id, _sub_id )
    local data_str = do_get_system_global_data( _system_id, _sub_id ) or "{}"
    dbserver.remote_call_game( "dbclient.on_get_system_global_data", _system_id, _sub_id, data_str )
end

function save_system_global_data( _system_id, _sub_id, _data_str )
    local log_head = sformat( "%s(get_system_global_data)", LOG_HEAD )
    if _sub_id >= share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID then
        c_err( "%s sub_id: %d is exceed max value: %d, system_id: %d", log_head, _sub_id, share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID, _system_id )
        return false
    end

    local data_id = get_data_id( _system_id, _sub_id )
    if dbserver.g_db_character:c_stmt_format_modify( REPLACE_SYSTEM_GLOBAL_DATA_SQL, REPLACE_SYSTEM_GLOBAL_DATA_PARAM_TYPE, data_id, _data_str ) < 0 then
        c_err( "%s c_stmt_format_query on REPLACE_SYSTEM_GLOBAL_DATA_SQL failed, system_id: %d, sub_id: %d", log_head, _system_id, _sub_id )
        return false
    end
    return true
end

function delete_system_global_data( _system_id, _sub_id )
    local log_head = sformat( "%s(delete_system_global_data)", LOG_HEAD )
    if _sub_id >= share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID then
        c_err( "%s sub_id: %d is exceed max value: %d, system_id: %d", log_head, _sub_id, share_const.SYSTEM_GLOBAL_DATA_TBL_MAX_SUB_ID, _system_id )
        return false
    end

    local data_id = get_data_id( _system_id, _sub_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( DELETE_SYSTEM_GLOBAL_DATA_SQL, DELETE_SYSTEM_GLOBAL_DATA_SQL_PARAM_TYPE, data_id ) < 0 then
        c_err( "%s delete data db error! team_id: %d", log_head, data_id )
        return false
    end
    return true
end

