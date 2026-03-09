module( "db_player_offline_op", package.seeall )

local os_time, tinsert = os.time, table.insert
g_delete_expired_op_timer = g_delete_expired_op_timer or nil 

SELECT_PLAYER_OFFLINE_OP_SQL = [[
    SELECT 
    `op_id`,
    `player_id`,
    `expire_os_time`,
    `op_func`,
    `op_func_param`
    from PLAYER_OFFLINE_OP_TBL 
    where player_id = ? 
    order by op_id; 
]]
SELECT_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE = "i"
SELECT_PLAYER_OFFLINE_OP_SQL_RESULT_TYPE = "liiss"

INSERT_PLAYER_OFFLINE_OP_SQL = [[
    INSERT into PLAYER_OFFLINE_OP_TBL 
    ( 
    `player_id`,
    `expire_os_time`,
    `op_func`,
    `op_func_param`
    ) 
    VALUES 
    ( ?, ?, ?, ?);
]]
INSERT_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE = "iiss"

DELETE_PLAYER_OFFLINE_OP_SQL = [[
    DELETE from PLAYER_OFFLINE_OP_TBL 
    where player_id = ? and op_id <= ?;
]]
DELETE_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE = "il"

DELETE_EXPIRED_PLAYER_OFFLINE_OP_SQL = [[
    DELETE from PLAYER_OFFLINE_OP_TBL 
    WHERE (expire_os_time > 0 AND expire_os_time < ?) 
        or not exists( SELECT `player_id` from PLAYER_TBL where player_id = PLAYER_OFFLINE_OP_TBL.player_id and not deleted);
]]
DELETE_EXPIRED_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE = "i"

-- ------------------------------------
-- Local Functions
-- ------------------------------------

function save_player_offline_op( _player_id, _expire_os_time, _op_func, op_func_param )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( INSERT_PLAYER_OFFLINE_OP_SQL, INSERT_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE, 
                                _player_id, _expire_os_time, _op_func, op_func_param) < 0 then
        c_err( "[db_player_offline_op](save_player_offline_op)failed to insert offline op, player_id:%d, expire:%d, func:[%s], param:[%s]", _player_id, _expire_os_time, _op_func, op_func_param )
        return 
    end

    time = c_cpu_ms() - time
    c_prof( "[db_player_offline_op](save_player_offline_op)player_id:%d, op_func:%s time: %dms", _player_id, _op_func, time )
end

function delete_player_op_ids( _player_id, _max_op_id )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( 
        DELETE_PLAYER_OFFLINE_OP_SQL, 
        DELETE_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE, _player_id, _max_op_id) < 0 then
        c_err( "[db_player_offline_op](delete_player_op_ids)failed to delete , player_id:%d, max_id:%d", _player_id, _max_op_id )
        return 
    end

    time = c_cpu_ms() - time
    c_prof( "[db_player_offline_op](delete_player_op_ids)player_id:%d, max_op_id:%d, use time: %dms", _player_id, _max_op_id, time )
end

function delete_expired_op( )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    local now = os_time()
    local affect_nows = database:c_stmt_format_modify( 
                DELETE_EXPIRED_PLAYER_OFFLINE_OP_SQL, DELETE_EXPIRED_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE, now)
    if affect_nows < 0 then
        c_err( "[db_player_offline_op](delete_expired_op)failed to delete expire, now:%d", now )
        return
    end

    time = c_cpu_ms() - time
    c_prof( "[db_player_offline_op](delete_expired_op) delete rows:%d, use time: %dms", affect_nows, time )
end

function get_player_offline_ops( _player_id )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PLAYER_OFFLINE_OP_SQL, 
        SELECT_PLAYER_OFFLINE_OP_SQL_PARAM_TYPE, _player_id, 
        SELECT_PLAYER_OFFLINE_OP_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_player_offline_op](get_player_offline_ops)  SELECT_PLAYER_OFFLINE_OP_SQL failed, player_id:%d", _player_id )
        return nil
    end

    local all_ops = {}
    local rows = database:c_stmt_row_num()
    if rows == 0 then return nil end

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local op_id, player_id, expire_os_time, op_func, op_func_param
                = database:c_stmt_get( "op_id", "player_id", "expire_os_time", "op_func", "op_func_param" )
            tinsert( all_ops, { op_id, expire_os_time, op_func, op_func_param })
        else
            c_err( "[db_player_offline_op](get_player_offline_ops) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end

    time = c_cpu_ms() - time
    c_prof( "[db_player_offline_op](get_player_offline_ops)rows:%d, use time: %dms", rows, time )

    return all_ops
end


-- ------------------------------------
-- Logic Functions
-- ------------------------------------
function do_start_delete_offline_timer()
    if not g_delete_expired_op_timer then
        g_delete_expired_op_timer = g_timermng:c_add_timer_second( 
            1, timers.CLEAR_OFFLINE_OPS, 0, 0, 300)
    end
end

function do_get_player_offline_ops( _player_id )
    local all_ops = get_player_offline_ops( _player_id )
    if all_ops and #all_ops > 0 then
        dbserver.remote_call_game( "player_gs_offline_op_mng.on_get_offline_op_from_db", _player_id, all_ops )
    end
    dbserver.remote_call_game( "player_gs_offline_op_mng.on_get_offline_op_from_db_done", _player_id )
end
do_delete_expired_op = delete_expired_op
do_save_player_offline_op = save_player_offline_op
do_delete_player_op_ids = delete_player_op_ids
