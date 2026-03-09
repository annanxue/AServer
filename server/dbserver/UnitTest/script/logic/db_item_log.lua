module( "db_item_log", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert



--一个月一张表
--表名：ITEM_LOG_TBL_201809

----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_ACTIVITY_STR = "{}"

SELECT_ITEM_LOG_SQL = [[
    SELECT
    `player_id`,
    `item_id`,	
    `num_before`,
    `num_after`,
    `num_delta`,	
    `time`,
    `sys_log`
    from ITEM_LOG_TBL_%s where player_id = %d %s and time between %d and %d;
]]

SELECT_ITEM_LOG_SQL_PARAM_TYPE = "iii"
SELECT_ITEM_LOG_SQL_RESULT_TYPE = "isiifis"



SAVE_ITEM_LOG_SQL = [[
    insert into %s (`player_id`, `item_id`, `num_before`, `num_after`, `num_delta`, `time`, `sys_log`)
    values(?, ?, ?, ?, ?, ?, ?);
]]
SAVE_ITEM_LOG_SQL_PARAM_TYPE = "isiiiis"

CREATE_ITEM_LOG_TBL_SQL =[[

    CREATE TABLE IF NOT EXISTS `%s` (
	`player_id`				int(11) NOT NULL default '0',
    `item_id`				varchar(32) NOT NULL,
	`num_before`			int(11) NOT NULL default '0',
	`num_after`				int(11) NOT NULL default '0',
	`num_delta`				int(11) NOT NULL default '0',
	`time`					int(11) NOT NULL default '0',
    `sys_log`				text collate latin1_bin NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_bin;

]]

LAST_CREATE_DB_MONTH = LAST_CREATE_DB_MONTH or 0

MAX_GET_COUNT_ONCE           = 500         -- 单次最大查询的数量

----------------------------------------------------
--                  local
----------------------------------------------------

function get_last_month_name()

    local this_year = tonumber(os.date("%Y", os.time()))
    local this_month = tonumber(os.date("%m", os.time()))

    if this_month == 1 then
        return string.format("%d%02d", this_year - 1, 12)
    else
        return string.format("%d%02d", this_year, this_month - 1)
    end
end

function do_get_player_item_log(_player_id, _item_id, _begin_time, _end_time)

    -- 判断需要查哪几张表
    local count = 0
    local tbls = {}

    local this_year = tonumber(os.date("%Y", os.time()))
    local this_month = tonumber(os.date("%m", os.time()))
    local this_month_start = os.time({day = 1, month = this_month, year = this_year, hour = 0, minute = 0, second = 0})
    if _begin_time < this_month_start then
        table.insert(tbls, get_last_month_name())
    end

    if this_month_start <= _end_time then
        table.insert(tbls, os.date("%Y%m", os.time()))
    end

    local database = dbserver.g_db_character

    local item_log = {}
    for _,v in ipairs(tbls) do

        local sql = string.format(SELECT_ITEM_LOG_SQL, v, _player_id, _item_id == '' and '' or 'and item_id = '..'"'.._item_id..'"', _begin_time, _end_time)

--        if database:c_stmt_format_query( sql, SELECT_ITEM_LOG_SQL_PARAM_TYPE, _player_id, _begin_time, _end_time, SELECT_ITEM_LOG_SQL_RESULT_TYPE ) >= 0 then
        if database:c_query( sql) >= 0 then

            local rows = database:c_row_num()
            count = rows

            if count > MAX_GET_COUNT_ONCE then
                return count, item_log
            end

            if rows >= 1 then
                for i = 1, rows, 1 do
                    if database:c_fetch() == 0 then
--                        local player_id, item_id, num_before, num_after, num_delta, time, sys_log
--                        = database:c_stmt_get( "player_id", "item_id", "num_before", "num_after", "num_delta", "time", "sys_log")

                        local r1, player_id = database:c_get_int32( "player_id" )
                        local r2, item_id = database:c_get_str( "item_id", 1024 )
                        local r3, num_before = database:c_get_int32( "num_before" )
                        local r4, num_after = database:c_get_int32( "num_after" )
                        local r5, num_delta = database:c_get_int32( "num_delta" )
                        local r6, time = database:c_get_int32( "time" )
                        local r7, sys_log = database:c_get_str( "sys_log", 1024 )

                        if r1 < 0 or r2 < 0 or r3 < 0 or r4 < 0 or r5 < 0 or r6 < 0 or r7 < 0 then
                            c_err("[db_item_log](do_get_player_item_log) c_get_xxx error")
                            return {}
                        end

                        tinsert(item_log, { player_id_ = player_id, item_id_ = item_id, num_before_ = num_before, num_after_ = num_after,
                            num_delta_ = num_delta, time_ = time, sys_log_ = sys_log})
                    else
                        c_err( "[db_item_log](do_get_player_item_log) c_stmt_fetch failed, index:%d", i)
                    end
                end
            end
        else
            c_err( "[db_item_log](do_get_player_item_log) c_stmt_format_query failed, player id: %d, item id:%s", _player_id, _item_id)
        end
    end
    return count, item_log
end

-- 不存在就创建，存在不作任何操作
function do_create_item_log_tbl(_tbl_name)

    local sql = string.format(CREATE_ITEM_LOG_TBL_SQL, _tbl_name)
    return dbserver.g_db_character:c_modify(sql)

end

function do_save_player_item_log(_item_log)

    local tbl_name = 'ITEM_LOG_TBL_'..os.date("%Y%m", os.time())

    local this_month = tonumber(os.date("%m", os.time()))

    -- 避免重复执行
    if this_month ~= LAST_CREATE_DB_MONTH then
        -- 获取要写入数据的表名
        -- 不存在则创建
        if do_create_item_log_tbl(tbl_name) < 0 then
            c_err("create %s failed", tbl_name)
            return 0
        end

        LAST_CREATE_DB_MONTH = this_month
    end

    return dbserver.g_db_character:c_stmt_format_modify( string.format(SAVE_ITEM_LOG_SQL, tbl_name), SAVE_ITEM_LOG_SQL_PARAM_TYPE,
        _item_log.player_id_, tostring(_item_log.item_id_), _item_log.num_before_, _item_log.num_after_, _item_log.num_delta_, _item_log.time_, _item_log.sys_log_)

end

