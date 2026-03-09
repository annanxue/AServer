module( "db_service", package.seeall )

local sformat, unpack = string.format, unpack

SELECT_LOGIN_SQL = [[
    SELECT
    `machine_code`,
    `login_token`, 
    `queue_free`, 
    `forbid_login`,
    `channel_account`,
    `channel_id`,
    `channel_label`,
    `is_prev_test_user`,
    `return_bind_diamond`, 
    `return_non_bind_diamond`, 
    `device_name`,
    `system_name`
    from LOGIN_TBL where account_id = ?;
]]
SELECT_LOGIN_SQL_RESULT_TYPE = "siiisssiiiss"

REPLACE_LOGIN_SQL = [[
    INSERT INTO LOGIN_TBL 
    ( `account_id`, `machine_code`, `login_token`, `channel_account`, `channel_id`, `channel_label`, `device_name`, `system_name` ) VALUES
    (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?
    )
    ON DUPLICATE KEY UPDATE
    machine_code = VALUES(machine_code),
    login_token = VALUES(login_token),
    channel_account = VALUES(channel_account),
    channel_id = VALUES(channel_id),
    channel_label = VALUES(channel_label), 
    device_name = VALUES(device_name),
    system_name = VALUES(system_name);
]]
REPLACE_LOGIN_SQL_PARAM_TYPE = "Lsisisss"

UPDATE_LOGIN_SQL = [[
    UPDATE LOGIN_TBL set login_token = ? where account_id = ?;
]]
UPDATE_LOGIN_SQL_PARAM_TYPE = "iL"

UPDATE_FORBID_ACCOUNT_ID_SQL = [[
    UPDATE LOGIN_TBL SET `forbid_login` = ? WHERE `account_id` = ?;
]]

UPDATE_FORBID_SACCOUNT_ID_SQL_PARAM_TYPE = "iL"

UPDATE_FORBID_machine_CODE_SQL = [[
    UPDATE LOGIN_TBL set `forbid_login` = ? WHERE `machine_code` = ?;
]]

UPDATE_FORBID_machine_CODE_SQL_PARAM_TYPE = "is"

CLEAR_RETURN_DIAMOND_SQL = [[
    UPDATE LOGIN_TBL SET return_bind_diamond = 0, return_non_bind_diamond = 0 WHERE account_id = %d;
]]

SELECT_LOGIN_MACHINE_CODE_SQL = [[
    SELECT
    `account_id`
    from LOGIN_TBL where machine_code = ?;
]]

SELECT_LOGIN_MACHINE_CODE_SQL_RESULT_TYPE = "i"

REPLACE_FORBID_MACHINE_SQL = [[
    INSERT INTO FORBID_MACHINE_TBL 
    ( `machine_code`, `forbid_end_time` ) VALUES
    (
        ?,
        ?
    )
    ON DUPLICATE KEY UPDATE
    forbid_end_time = VALUES(forbid_end_time);
]]
REPLACE_FORBID_MACHINE_SQL_PARAM_TYPE = "si"


SELECT_FORBID_TIME_BY_MACHINE_CODE_SQL  = [[
    SELECT
    `forbid_end_time`
    FROM FORBID_MACHINE_TBL WHERE machine_code = ?;
]]

SELECT_FORBID_TIME_BY_MACHINE_CODE_SQL_RESULT_TYPE = "i"

SELECT_ACTIVE_INFO_BY_ACCOUNT_ID_SERVER_ID = [[
    SELECT
    `active_code`
    FROM ACTIVE_CODE WHERE server_id = ? AND account_id = ?;
]]

SELECT_ACTIVE_INFO_BY_CODE = [[
    SELECT
    `server_id`,
    `account_id`
    FROM ACTIVE_CODE WHERE active_code = ?;
]]

UPDATE_ACTIVE_INFO = [[
    UPDATE ACTIVE_CODE SET `account_id` = ? where `active_code` =?;
]]



SELECT_ALL_FORBID_ACCOUNT_SQL = [[
    SELECT
    `account_id`,
    `forbid_login`
    from LOGIN_TBL where forbid_login > unix_timestamp(now());
]]
SELECT_ALL_FORBID_ACCOUNT_SQL_RESULT_TYPE = "ii"

function init()
    if ( g_db_login ) then 
        return true
    end
    g_db_login = DataBase:c_new() 
    local host, user, passwd, port, dbname = c_read_mysql( "db_login" )
    if( g_db_login:c_connect( host, user, passwd, port ) < 0 ) then
        c_err( "(init) connect database test failed.".. "[" .. host .. ":" .. port .."]".. user )
        return false
    end
    if g_db_login:c_select_db( dbname ) ~= 0 then
        c_err( "(init) select database %s failed", dbname )
        return false
    end
    return true 
end
assert( init() )

function on_exit()
    if (g_db_login) then
        g_db_login:c_delete()
        g_db_login = nil
    end
end

--------------------------------------------------
-- db interface 
--------------------------------------------------
function get_account_info_by_machine_code( _machine_code )
    if g_db_login:c_stmt_format_query( SELECT_LOGIN_MACHINE_CODE_SQL, "s", _machine_code, SELECT_LOGIN_MACHINE_CODE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_service](get_account_info_by_machine_code) failded to get account info, sql error. machine_code: %s", _machine_code )
        return false
    end
    local rows = g_db_login:c_stmt_row_num()
    if rows == 0 then
        c_trace( "[db_service](get_account_info_by_machine_code) no account info find by machine_code: %s", _machine_code )
        return false
    end

    local account_id_tbl = {}

    for i = 1, rows do
        if g_db_login:c_stmt_fetch() == 0 then
            local account_id = g_db_login:c_stmt_get("account_id")
            table.insert( account_id_tbl, account_id )
        else
            c_err("[db_service](get_account_info_by_machine_code) fetch error")
        end
    end

    return account_id_tbl
end

function get_machine_forbid_end_time( _machine_code )
    if g_db_login:c_stmt_format_query( SELECT_FORBID_TIME_BY_MACHINE_CODE_SQL, "s", _machine_code, SELECT_FORBID_TIME_BY_MACHINE_CODE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_service](get_forbid_time_by_machine_code) faild to get forbid time, sql error, machine_code: %s", _machine_code )
        return 0 
    end

    local rows = g_db_login:c_stmt_row_num()
    if rows == 0 then
        return 0
    end

    if g_db_login:c_stmt_fetch() == 0 then
        local forbid_end_time = g_db_login:c_stmt_get("forbid_end_time")
        return forbid_end_time
    end

    return 0
end

function get_account_info( _account_id )
    if g_db_login:c_stmt_format_query( SELECT_LOGIN_SQL, "L", _account_id, SELECT_LOGIN_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_service](get_account_info)failed to get account info, sql error. account_id: %s", _account_id )
        return false
    end

    local rows = g_db_login:c_stmt_row_num()
    if rows == 0 then
        c_trace( "[db_service](get_account_info)failed to get account info, account not exsit. account_id: %s", _account_id )
        return false
    end
  
    if g_db_login:c_stmt_fetch() < 0 then
        c_err( "[db_service](get_account_info)failed to get account info, stmt_fetch error. account_id: %s", _account_id )
        return false
    end

    local account_info =
    {
        account_id_ = _account_id,
        machine_code_ = g_db_login:c_stmt_get( "machine_code" ),
        login_token_ = g_db_login:c_stmt_get( "login_token" ),
        queue_free_ = g_db_login:c_stmt_get( "queue_free" ),
        forbid_login_ = g_db_login:c_stmt_get( "forbid_login" ),
        channel_account_ = g_db_login:c_stmt_get( "channel_account" ),
        channel_id_ = g_db_login:c_stmt_get( "channel_id" ),
        channel_label_ = g_db_login:c_stmt_get( "channel_label" ),
        is_prev_test_user_ = g_db_login:c_stmt_get( "is_prev_test_user" ) ~= 0,
        return_bind_diamond_ = g_db_login:c_stmt_get( "return_bind_diamond" ),
        return_non_bind_diamond_ = g_db_login:c_stmt_get( "return_non_bind_diamond" ),
        device_name_ = g_db_login:c_stmt_get( "device_name" ),
        system_name_ =  g_db_login:c_stmt_get( "system_name" ),
    }
    return true, account_info
end

function get_active_info_by_account_id( _account_id, _server_id )
    if g_db_login:c_stmt_format_query( SELECT_ACTIVE_INFO_BY_ACCOUNT_ID_SERVER_ID, "iL", _server_id, _account_id, "s" ) < 0 then
        c_err("[db_service](get_active_info_by_account_id) faild to get active info, sql error, account_id:%s, server_id:%d", _account_id, _server_id)
        return false
    end

    local rows = g_db_login:c_stmt_row_num()
    if rows == 0 then
        c_trace( "[db_service](get_active_info_by_account_id)no active info. account_id: %s, server_id:%d", _account_id, _server_id )
        return false
    end

    if g_db_login:c_stmt_fetch() < 0 then
        c_err( "[db_service](get_active_info_by_account_id)failed to get active info, stmt_fetch error. account_id: %s, server_id: %d", _account_id, _server_id )
        return false
    end
    
    local active_code = g_db_login:c_stmt_get("active_code")

    return true, active_code
end

function get_active_info_by_code( _active_code )
    if g_db_login:c_stmt_format_query( SELECT_ACTIVE_INFO_BY_CODE, "s", _active_code, "iL" ) < 0 then
        c_err("[db_service](get_active_info_by_code) faild to get active info, sql error, active_code:%s", _active_code)
        return false
    end

    local rows = g_db_login:c_stmt_row_num()
    if rows == 0 then
        c_trace( "[db_service](get_active_info_by_account_id)no active info. active_code:%s", _active_code )
        return false
    end

    if g_db_login:c_stmt_fetch() < 0 then
        c_err( "[db_service](get_active_info_by_account_id) failed to get active info, stmt_fetch error, active_code:%s", _active_code )
        return false
    end

    local server_id = g_db_login:c_stmt_get("server_id")
    local account_id = g_db_login:c_stmt_get("account_id")

    return true, server_id, account_id
end

function update_active_info( _account_id, _active_code )
    if g_db_login:c_stmt_format_modify( UPDATE_ACTIVE_INFO,"Ls", _account_id, _active_code ) < 0 then
        c_err("[db_service](get_active_info_by_account_id) faild to update active info, active_code:%s, account_id:%s", _active_code, _account_id)
        return false
    end

    c_log("[db_service](get_active_info_by_account_id) update active info set active_code:%s, account_id:%s", _active_code, _account_id)
    return true
end

function update_account_info( _account_id, _mcode, _login_token, _channel_account, _channel_id, _channel_label, _device_name, _system_name )
    local time = c_cpu_ms()
    local ret_code = g_db_login:c_stmt_format_modify( REPLACE_LOGIN_SQL, REPLACE_LOGIN_SQL_PARAM_TYPE, 
        _account_id, _mcode, _login_token, _channel_account, _channel_id, _channel_label, _device_name, _system_name )
    time = c_cpu_ms() - time
    c_prof( "[db_service](update_account_info)update LOGIN_TBL use time: %dms, account_id: %s", time, _account_id )
    return ret_code
end

function invalid_account_token( _account_id )
    local ret_code = g_db_login:c_stmt_format_modify( UPDATE_LOGIN_SQL, UPDATE_LOGIN_SQL_PARAM_TYPE, 0, _account_id )
    return ret_code
end

function update_forbid_machine_info( _machine_code, _forbid_end_time )
    local ret_code = g_db_login:c_stmt_format_modify( REPLACE_FORBID_MACHINE_SQL, REPLACE_FORBID_MACHINE_SQL_PARAM_TYPE, _machine_code, _forbid_end_time)
    if ret_code < 0 then
        c_err("[db_service](update_forbid_machine_info) REPLACE_FORBID_MACHINE_SQL error, machine_code:%s, forbid_end_time:%d", _machine_code, _forbid_end_time)
        return false
    end
    return true
end

function update_forbid_info( _account_id, _forbid_end_time )
    if is_new_account(_account_id) then
        local err_msg = sformat( "[db_service](update_forbid_info) account_id:%d not exist", _account_id)
        c_err(err_msg)
        return false, err_msg 
    end

    local ret_code = g_db_login:c_stmt_format_modify( UPDATE_FORBID_ACCOUNT_ID_SQL, UPDATE_FORBID_SACCOUNT_ID_SQL_PARAM_TYPE, _forbid_end_time, _account_id)
    if ret_code < 0 then
        local err_msg = sformat( "[db_service](update_forbid_info) UPDATE_FORBID_ACCOUNT_ID_SQL err, account_id: %d, forbid_end_time: %d", _account_id, _forbid_end_time )
        return false, err_msg
    end
    return true 
end

function get_account_with_same_machine_code( _account_id )
    local machine_code = ""
    if is_new_account( _account_id ) then
        return machine_code
    end

    local ret, account_info = get_account_info( _account_id )
    if ret then
        machine_code = account_info.machine_code_
    end

    return machine_code
end

function is_new_account( _account_id )
    if g_db_login:c_stmt_format_query( SELECT_LOGIN_SQL, "L", _account_id, SELECT_LOGIN_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_service](is_new_account)failed to get login info, sql error. account_id: %s", _account_id )
        return false
    end

    local rows = g_db_login:c_stmt_row_num()
    return rows == 0 
end

function clear_return_diamond( _account_id, game_server_id, _player_id )
    if g_db_login:c_modify( sformat( CLEAR_RETURN_DIAMOND_SQL, _account_id ) ) < 0 then
        c_err( "[db_service](clear_return_diamond) CLEAR_RETURN_DIAMOND_SQL exec error, account_id: %d", _account_id )
        return false
    end
    c_log( "[db_service](clear_return_diamond) account_id: %d, game_server_id: %d, player_id: %d", _account_id, game_server_id, _player_id )
    return true
end

function get_all_forbid_account()

    if g_db_login:c_stmt_format_query( SELECT_ALL_FORBID_ACCOUNT_SQL, "", SELECT_ALL_FORBID_ACCOUNT_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_service](get_all_forbid_account)  SELECT_ALL_FORBID_ACCOUNT_SQL failed" )
        return nil
    end

    local all_forbid_account = {}
    local rows = g_db_login:c_stmt_row_num()

    for i = 1, rows, 1 do
        if g_db_login:c_stmt_fetch() == 0 then
            local forbid_account = {}
            forbid_account.account_id, forbid_account.forbid_login = g_db_login:c_stmt_get( "account_id", "forbid_login")

            forbid_account.forbid_login = os.date("%Y-%m-%d %H:%M:%S", forbid_account.forbid_login)

            table.insert(all_forbid_account, forbid_account)
        else
            c_err( "[db_service](get_all_forbid_account) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end
    c_log("[db_service](get_all_forbid_account) num:%d", #all_forbid_account)
    return all_forbid_account
end

