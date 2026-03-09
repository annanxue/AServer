module( "db_activity", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert


ACTIVITY_KEY_LIST = 
{
    'total_recharge_reward',
    'single_recharge_reward',
    'continuous_recharge_reward',
    'logon_reward',
    'vitality_reward',
    'consume_reward',
    'exchange_reward',
    'hot_sale_reward',
    'popup_poster',
    'time_limit_discount',
    'recharge_mall',
    'festival_task',
    'logon_wish',
}

ACTIVITY_PARAM_KEY_LIST = 
{
    'total_recharge_reward_param',
    'single_recharge_reward_param',
    'continuous_recharge_reward_param',
    'logon_reward_param',
    'vitality_reward_param',
    'consume_reward_param',
    'exchange_reward_param',
    'hot_sale_reward_param',
    'popup_poster_param',
    'time_limit_discount_param',
    'recharge_mall_param',
    'festival_task_param',
    'logon_wish_param',
}


local function init_select_activity_sql()
    local select_activity_sql = [[
        SELECT
        `%s`
        from ACTIVITY_TBL where player_id = ?;
    ]]
    return string.format(select_activity_sql, table.concat(ACTIVITY_KEY_LIST, '`,`'))
end

local function init_select_activity_sql_result_type()
    local tbl = {}
    for i = 1, #ACTIVITY_KEY_LIST do
        table.insert(tbl, 's')
    end
    return table.concat(tbl, '')
end

local function init_select_activity_param_sql()
    local select_activity_param_sql = [[
        SELECT
        `%s`
        from ACTIVITY_PARAM_TBL;
    ]]
    return string.format(select_activity_param_sql, table.concat(ACTIVITY_PARAM_KEY_LIST, '`,`'))
end

local function init_select_activity_param_sql_result_type()
    local tbl = {}
    for i = 1, #ACTIVITY_PARAM_KEY_LIST do
        table.insert(tbl, 's')
    end
    return table.concat(tbl, '')
end


local function init_save_all_activity_sql_format()
    local save_all_activity_sql_format = [[
        REPLACE INTO ACTIVITY_TBL ( `player_id`, `%s` ) VALUES 
        (
            ?,
            %s
        )
    ]]
    local values = {}
    for i = 1, #ACTIVITY_KEY_LIST do
        table.insert(values, '?')
    end

    return string.format(save_all_activity_sql_format, table.concat(ACTIVITY_KEY_LIST, '`,`'), table.concat(values, ','))

end

local function init_save_all_activity_sql_type()
    local tbl = {}
    for i = 1, #ACTIVITY_KEY_LIST do
        table.insert(tbl, 's')
    end
    return string.format('i%s', table.concat(tbl, ''))
end


----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_ACTIVITY_STR = "{}"

SELECT_ACTIVITY_SQL = init_select_activity_sql()
SELECT_ACTIVITY_SQL_RESULT_TYPE = init_select_activity_sql_result_type()


SELECT_ACTIVITY_PARAM_SQL = init_select_activity_param_sql()


SELECT_ACTIVITY_PARAM_SQL_PARAM_TYPE = ""
SELECT_ACTIVITY_PARAM_SQL_RESULT_TYPE = init_select_activity_param_sql_result_type()


INSERT_ACTIVITY_PARAM_SQL = [[
    insert into ACTIVITY_PARAM_TBL(id, total_recharge_reward_param) values(0, "");
]]
INSERT_ACTIVITY_PARAM_SQL_PARAM_TYPE = ""

--REPLACE_ACTIVITY_PARAM_SQL_FORMAT = [[
--    REPLACE INTO ACTIVITY_PARAM_TBL (`%s`) VALUES 
--    (
--        ?
--    )
--]]
--REPLACE_ACTIVITY_PARAM_SQL_TYPE = "s"

UPDATE_ACTIVITY_PARAM_SQL_FORMAT = [[
    UPDATE ACTIVITY_PARAM_TBL SET %s = ? where id = 0;
]]
UPDATE_ACTIVITY_PARAM_SQL_TYPE = "s"



SAVE_ALL_ACTIVITY_SQL = init_save_all_activity_sql_format()
SAVE_ALL_ACTIVITY_SQL_TYPE = init_save_all_activity_sql_type()

RESET_ACTIVITY_SQL = 'UPDATE ACTIVITY_TBL SET %s = "do local ret={} return ret end";'



INSERT_ACTIVITY_PARAM_LOG_SQL = [[
    insert into ACTIVITY_PARAM_LOG_TBL(type, begin_time, end_time, stage_count, activity_param) values(?, ?, ?, ?, ?);
]]
INSERT_ACTIVITY_PARAM_LOG_SQL_TYPE = "iiiis"

SELECT_ALL_ACTIVITY_PARAM_LOG_SQL = [[
    SELECT `type`, `begin_time`, `end_time`, `stage_count`, `activity_param` from ACTIVITY_PARAM_LOG_TBL where end_time >= ?;
]]
SELECT_ALL_ACTIVITY_PARAM_LOG_SQL_TYPE_PARAM = "i"
SELECT_ALL_ACTIVITY_PARAM_LOG_SQL_RESULT_TYPE = "iiiis"

SELECT_ACTIVITY_PARAM_LOG_SQL = [[
    select `type`, `activity_param` from ACTIVITY_PARAM_LOG_TBL where type in (?) and begin_time < ? and ? < end_time;
]]
SELECT_ACTIVITY_PARAM_LOG_SQL_PARAM_TYPE = "sii"
SELECT_ACTIVITY_PARAM_LOG_SQL_RESULT_TYPE = "is"


----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_activity( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ACTIVITY_SQL, "i", _player_id, SELECT_ACTIVITY_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_activity](do_get_player_acitivty) c_stmt_format_query failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
--        return DEFAULT_ACTIVITY_STR
        local activity_str_tbl = {}
        for i = 1, #ACTIVITY_KEY_LIST do
            table.insert(activity_str_tbl, utils.serialize_table({}))
        end
--        return unpack(activity_str_tbl)
        return activity_str_tbl
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local activity_str_tbl = {}
            for i = 1, #ACTIVITY_KEY_LIST do
                local activity_str = database:c_stmt_get(ACTIVITY_KEY_LIST[i])
                activity_str = (activity_str and activity_str ~= "") and activity_str or utils.serialize_table({})
                table.insert(activity_str_tbl, activity_str)
            end
--            return unpack(activity_str_tbl)
            return activity_str_tbl
        end
    else
        c_err("do_get_player_acitivty err. rows:%s", rows)
    end
    return nil
end

function do_save_player_activity( _player )

    local param_tbl = {}
    for _,v in ipairs(ACTIVITY_KEY_LIST) do
        table.insert(param_tbl, _player[v.."_"])
    end
    return dbserver.g_db_character:c_stmt_format_modify( SAVE_ALL_ACTIVITY_SQL, SAVE_ALL_ACTIVITY_SQL_TYPE,
            _player.player_id_, unpack(param_tbl))

end

function do_get_activity_param_all()

    local activity_param = {}

    local database = dbserver.g_db_character
    if database:c_stmt_format_query(SELECT_ACTIVITY_PARAM_SQL, SELECT_ACTIVITY_PARAM_SQL_PARAM_TYPE, SELECT_ACTIVITY_PARAM_SQL_RESULT_TYPE) < 0 then
        c_err( "[db_activity](do_get_activity_param_all) c_stmt_format_query failed")
        return
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            for _,v in ipairs(ACTIVITY_PARAM_KEY_LIST) do
                dbserver.remote_call_game("activity_mng.on_get_activity_param_from_db", v, database:c_stmt_get(v))
            end
            return
        end
    else
        c_err("do_get_player_acitivty err. rows:%s", rows)
    end
end

local function get_all_activity_param()
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ACTIVITY_PARAM_SQL, SELECT_ACTIVITY_PARAM_SQL_PARAM_TYPE, SELECT_ACTIVITY_PARAM_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_arena](get_all_activity_param)  SELECT_ACTIVITY_PARAM_SQL failed" )
        return nil
    end

    local all_activity_param = {}
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return {}
    end

    if database:c_stmt_fetch() == 0 then

        for _,v in ipairs(ACTIVITY_PARAM_KEY_LIST) do
            all_activity_param[v] = database:c_stmt_get(v)
        end
    else
        c_err( "[db_activity](get_all_activity_param) c_stmt_fetch failed")
        return nil
    end

    time = c_cpu_ms() - time
    c_prof( "[db_activity](get_all_activity_param)use time: %dms", time )

    return all_activity_param
end

function do_save_activity_param(_activity_type, _activity_param_str)

     -- 避免建表的时候忘记插入一行
    local all_activity_param = get_all_activity_param()
    if not all_activity_param or utils.table.size(all_activity_param) == 0 then
        local affected_rows = dbserver.g_db_character:c_stmt_format_modify(
            INSERT_ACTIVITY_PARAM_SQL, INSERT_ACTIVITY_PARAM_SQL_PARAM_TYPE)
        if affected_rows < 0 then
            c_err("INSERT_ACTIVITY_PARAM_SQL err:%d", affected_rows)
            return
        end
    end
    
    local db_key = ACTIVITY_PARAM_KEY_LIST[_activity_type]
    local sql = string.format(UPDATE_ACTIVITY_PARAM_SQL_FORMAT, db_key)

    return dbserver.g_db_character:c_stmt_format_modify(
            sql, UPDATE_ACTIVITY_PARAM_SQL_TYPE, _activity_param_str)
end

function do_get_all_activity_param()
    
    local all_activity_param = get_all_activity_param()
    c_assert( all_activity_param, "[db_activity](do_get_all_activity_param) failed")

    dbserver.remote_call_game( "activity_mng.on_get_all_activity_param", all_activity_param or {} )

end

function get_all_activity_param_log()
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_ACTIVITY_PARAM_LOG_SQL, SELECT_ALL_ACTIVITY_PARAM_LOG_SQL_TYPE_PARAM, os.time(), SELECT_ALL_ACTIVITY_PARAM_LOG_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_activity](get_all_activity_param_log)  SELECT_ALL_ACTIVITY_PARAM_LOG_SQL failed" )
        return nil
    end

    local all_activity_param = {}
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return {}
    end

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local type, begin_time, end_time, stage_count, activity_param = database:c_stmt_get( "type", "begin_time", "end_time", "stage_count", "activity_param")
            all_activity_param[type] = all_activity_param[type] or {}
            local key = begin_time.."_"..end_time
            all_activity_param[type][key] = loadstring(activity_param)()
        else
            c_err( "[db_activity](get_all_activity_param_log) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end

    time = c_cpu_ms() - time
    c_prof( "[db_activity](get_all_activity_param_log)use time: %dms", time )

    return all_activity_param
end

-- 获取当前时间有效的活动配置+所有活动的类型、开始时间、结束时间[web下发时校验]
function do_get_all_activity_param_log()

    local activity_param_log_time_list = get_all_activity_param_log()
    dbserver.remote_call_game("activity_mng.on_get_activity_param_log_valid", activity_param_log_time_list or {})

end

function do_save_activity_param_log(_activity_type, _begin_time, _end_time, _stage_count, _activity_param_str)

    return dbserver.g_db_character:c_stmt_format_modify(
        INSERT_ACTIVITY_PARAM_LOG_SQL, INSERT_ACTIVITY_PARAM_LOG_SQL_TYPE,
        _activity_type, _begin_time, _end_time, _stage_count, _activity_param_str)

end

