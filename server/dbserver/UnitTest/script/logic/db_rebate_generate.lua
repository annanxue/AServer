module( "db_rebate_generate", package.seeall )


---------------------------------------------------------------------------------------------------------------
--                                      生成REBATE_TBL工具
---------------------------------------------------------------------------------------------------------------

local function get_db_by_name( _db_name )
    local host = "127.0.0.1"
    local user = "root"
    local passwd = "root"
    local port = 3306

    local db = DataBase:c_new()

    if db:c_connect( host, user, passwd, port ) < 0 then
        return nil
    end

    if db:c_select_db( _db_name ) ~= 0 then
        return nil
    end

    return db
end

local function generate_recharge_data( _db, _player_map, _field_name )
    local sql = "select player_id, recharge_total from PLAYER_RMB_TBL;"

    if _db:c_query( sql ) < 0 then
        c_err( "[db_rebate_generate](generate_recharge_data)query error, sql: %s", sql )
        return false
    end
 
    local rows = _db:c_row_num()

    for i = 1, rows, 1 do
        if _db:c_fetch() ~= 0 then
            c_err( "[db_rebate_generate](generate_recharge_data)fetch error, sql: %s", sql )
            return false
        end

        local r1, player_id = _db:c_get_int32( "player_id" )
        if r1 < 0 then
            c_err( "[db_rebate_generate](generate_recharge_data)c_get_int32 player_id error, sql: %s", sql )
            return false
        end

        local r2, recharge_total = _db:c_get_int32( "recharge_total" )
        if r2 < 0 then
            c_err( "[db_rebate_generate](generate_recharge_data)c_get_int32 recharge_total error, sql: %s", sql )
            return false
        end

        local player_info = _player_map[player_id]
        recharge_total = math.floor( recharge_total/100 )
        if player_info then
            player_info[_field_name] = recharge_total
        else
            player_info = { [_field_name] = recharge_total }
            _player_map[player_id] = player_info
        end
    end
end

local function generate_gs_rank_data( _db, _player_map, _field_name )
    local sql = "select player_id, total_gs from PLAYER_TBL order by total_gs desc limit 100;"

    if _db:c_query( sql ) < 0 then
        c_err( "[db_rebate_generate](generate_gs_rank_data)query error, sql: %s", sql )
        return false
    end
 
    local rows = _db:c_row_num()

    for i = 1, rows, 1 do
        if _db:c_fetch() ~= 0 then
            c_err( "[db_rebate_generate](generate_gs_rank_data)fetch error, sql: %s", sql )
            return false
        end

        local r1, player_id = _db:c_get_int32( "player_id" )
        if r1 < 0 then
            c_err( "[db_rebate_generate](generate_gs_rank_data)c_get_int32 player_id error, sql: %s", sql )
            return false
        end

        local r2, total_gs = _db:c_get_int32( "total_gs" )
        if r2 < 0 then
            c_err( "[db_rebate_generate](generate_gs_rank_data)c_get_int32 total_gs error, sql: %s", sql )
            return false
        end

        local player_info = _player_map[player_id]
        if player_info then
            player_info[_field_name] = i
        else
            player_info = { [_field_name] = i }
            _player_map[player_id] = player_info
        end
    end
end

local function generate_level_data( _db, _player_map, _field_name )
    local sql = "select player_id, level from PLAYER_TBL;"

    if _db:c_query( sql ) < 0 then
        c_err( "[db_rebate_generate](generate_level_data)query error, sql: %s", sql )
        return false
    end
 
    local rows = _db:c_row_num()

    for i = 1, rows, 1 do
        if _db:c_fetch() ~= 0 then
            c_err( "[db_rebate_generate](generate_level_data)fetch error, sql: %s", sql )
            return false
        end

        local r1, player_id = _db:c_get_int32( "player_id" )
        if r1 < 0 then
            c_err( "[db_rebate_generate](generate_level_data)c_get_int32 player_id error, sql: %s", sql )
            return false
        end

        local r2, level = _db:c_get_int32( "level" )
        if r2 < 0 then
            c_err( "[db_rebate_generate](generate_level_data)c_get_int32 level error, sql: %s", sql )
            return false
        end

        local player_info = _player_map[player_id]
        if player_info then
            player_info[_field_name] = level
        else
            player_info = { [_field_name] = level }
            _player_map[player_id] = player_info
        end
    end
end

local function generate_login_days_data( _db, _player_map, _field_name )
    local sql = "select player_id, date_format(create_time, '%j') as create_time, date_format(last_update_time, '%j') as last_update_time from PLAYER_TBL;"

    if _db:c_query( sql ) < 0 then
        c_err( "[db_rebate_generate](generate_login_days_data)query error, sql: %s", sql )
        return false
    end

    local rows = _db:c_row_num()

    for i = 1, rows, 1 do
        if _db:c_fetch() ~= 0 then
            c_err( "[db_rebate_generate](generate_login_days_data)fetch error, sql: %s", sql )
            return false
        end

        local r1, player_id = _db:c_get_int32( "player_id" )
        if r1 < 0 then
            c_err( "[db_rebate_generate](generate_login_days_data)c_get_int32 player_id error, sql: %s", sql )
            return false
        end

        local r2, create_time = _db:c_get_str( "create_time", 64 )
        if r2 < 0 then
            c_err( "[db_rebate_generate](generate_login_days_data)c_get_str create_time error, sql: %s", sql )
            return false
        end

        local r3, last_update_time = _db:c_get_str( "last_update_time", 64 )
        if r3 < 0 then
            c_err( "[db_rebate_generate](generate_login_days_data)c_get_str last_update_time error, sql: %s", sql )
            return false
        end

        local login_days = tonumber( last_update_time ) - tonumber( create_time ) + 1

        local player_info = _player_map[player_id]
        if player_info then
            player_info[_field_name] = login_days
        else
            player_info = { [_field_name] = login_days }
            _player_map[player_id] = player_info
        end
    end
end

local function merge_one_data( _left, _right )
    local new_data = {}

    -- test1_recharge
    local test1_recharge_left = _left.test1_recharge or 0
    local test1_recharge_right = _right.test1_recharge or 0
    new_data.test1_recharge = test1_recharge_left + test1_recharge_right

    -- test1_gs_rank
    local test1_gs_rank_left = _left.test1_gs_rank or 0
    local test1_gs_rank_right = _right.test1_gs_rank or 0
    if test1_gs_rank_left > 0 and test1_gs_rank_right > 0 then
        if test1_gs_rank_right > test1_gs_rank_left then
            new_data.test1_gs_rank = test1_gs_rank_left
        else
            new_data.test1_gs_rank = test1_gs_rank_right
        end
    elseif test1_gs_rank_left > 0 then
        new_data.test1_gs_rank = test1_gs_rank_left
    elseif test1_gs_rank_right > 0 then
        new_data.test1_gs_rank = test1_gs_rank_right
    else
        new_data.test1_gs_rank = 0
    end

    -- test1_level
    local test1_level_left = _left.test1_level or 0
    local test1_level_right = _right.test1_level or 0
    if test1_level_right >= test1_level_left then
        new_data.test1_level = test1_level_right
    else
        new_data.test1_level = test1_level_left
    end

    -- test1_login_days
    local test1_login_days_left = _left.test1_login_days or 0
    local test1_login_days_right = _right.test1_login_days or 0
    if test1_login_days_right >= test1_login_days_left then
        new_data.test1_login_days = test1_login_days_right
    else
        new_data.test1_login_days = test1_login_days_left
    end

    -- test2_recharge
    local test2_recharge_left = _left.test2_recharge or 0
    local test2_recharge_right = _right.test2_recharge or 0
    new_data.test2_recharge = test2_recharge_left + test2_recharge_right

    -- test2_level
    local test2_level_left = _left.test2_level or 0
    local test2_level_right = _right.test2_level or 0
    if test2_level_right >= test2_level_left then
        new_data.test2_level = test2_level_right
    else
        new_data.test2_level = test2_level_left
    end

    -- test2_login_days
    local test2_login_days_left = _left.test2_login_days or 0
    local test2_login_days_right = _right.test2_login_days or 0
    if test2_login_days_right >= test2_login_days_left then
        new_data.test2_login_days = test2_login_days_right
    else
        new_data.test2_login_days = test2_login_days_left
    end

    return new_data
end

local function merge_player_data( _db, _player_map, _account_map )
    local sql = "select player_id, machine_code_id from PLAYER_TBL;"

    if _db:c_query( sql ) < 0 then
        c_err( "[db_rebate_generate](merge_player_data)query error, sql: %s", sql )
        return false
    end

    local rows = _db:c_row_num()

    local player_id_2_account_id = {}

    for i = 1, rows, 1 do
        if _db:c_fetch() ~= 0 then
            c_err( "[db_rebate_generate](merge_player_data)fetch error, sql: %s", sql )
            return false
        end

        local r1, player_id = _db:c_get_int32( "player_id" )
        if r1 < 0 then
            c_err( "[db_rebate_generate](merge_player_data)c_get_int32 player_id error, sql: %s", sql )
            return false
        end

        local r2, account_id = _db:c_get_int32( "machine_code_id" )
        if r2 < 0 then
            c_err( "[db_rebate_generate](merge_player_data)c_get_int32 account_id error, sql: %s", sql )
            return false
        end

        player_id_2_account_id[player_id] = account_id
    end

    for player_id, player_info in pairs( _player_map ) do
        local account_id = player_id_2_account_id[player_id]

        local account_info = _account_map[account_id]
        if account_info then
            _account_map[account_id] = merge_one_data( account_info, player_info )
        else
            _account_map[account_id] = player_info
        end
    end
end

local function fix_data( _account_map )
    for account_id, account_info in pairs( _account_map ) do
        account_info.test1_recharge = account_info.test1_recharge or 0
        account_info.test1_gs_rank = account_info.test1_gs_rank or 0
        account_info.test1_level = account_info.test1_level or 0
        account_info.test1_login_days = account_info.test1_login_days or 0
        account_info.test2_recharge = account_info.test2_recharge or 0
        account_info.test2_level = account_info.test2_level or 0
        account_info.test2_login_days = account_info.test2_login_days or 0
    end
end

local function merge_account_data( _test1_account_map, _test2_account_map )
    for account_id, account_info in pairs( _test1_account_map ) do
        local test2_account_info = _test2_account_map[account_id]
        if test2_account_info then
            test2_account_info.test1_recharge = account_info.test1_recharge
            test2_account_info.test1_gs_rank = account_info.test1_gs_rank
            test2_account_info.test1_level = account_info.test1_level
            test2_account_info.test1_login_days = account_info.test1_login_days
        else
            _test2_account_map[account_id] = account_info
            account_info.test2_recharge = 0
            account_info.test2_level = 0
            account_info.test2_login_days = 0
        end
    end

    return _test2_account_map
end

local function generate_insert_sql( _account_map )
    local sql_list = {}

    for account_id, account_info in pairs( _account_map ) do
        local sql = string.format( "insert into REBATE_TBL ( account_id, test1_recharge, test1_gs_rank, test1_level, test1_login_days, test2_recharge, test2_level,  test2_login_days,  is_return, confirm_player_id) values (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d);" , 
            account_id, account_info.test1_recharge, account_info.test1_gs_rank, account_info.test1_level, account_info.test1_login_days, account_info.test2_recharge, account_info.test2_level, account_info.test2_login_days, 0, 0 )
        table.insert( sql_list, sql )
    end

    return sql_list
end

local function insert_final_data( _db, _sql_list )
    local delete_sql = "delete from REBATE_TBL;"

    if _db:c_modify( delete_sql ) < 0 then
        c_err( "[rebate_mng](insert_final_data)c_modify err. sql: %s", delete_sql )
        return false
    end
  
    c_log( "---------------准备执行sql语句：%d条---------------", #_sql_list )

    for _, sql in ipairs( _sql_list ) do
        if _db:c_modify( sql ) < 0 then
            c_err( "[rebate_mng](insert_final_data)c_modify err. sql: %s", sql )
            return false
        end
    end

    c_log( "---------------------执行完毕----------------------" )
end

function generate_rebate_table()
    if not g_debug then
        return
    end

    local rebate_db = get_db_by_name( "test" )
    if not rebate_db then
        c_err( "[rebate_mng](generate_rebate_sql)can't get rebate_db" )
        return false
    end

    local test1_20001_db = get_db_by_name( "test1_20001" )
    if not test1_20001_db then
        c_err( "[rebate_mng](generate_rebate_sql)can't get test1_20001_db" )
        return false
    end

    local test1_30001_db = get_db_by_name( "test1_30001" )
    if not test1_30001_db then
        c_err( "[rebate_mng](generate_rebate_sql)can't get test1_30001_db" )
        return false
    end

    local test2_20002_db = get_db_by_name( "test2_20002" )
    if not test2_20002_db then
        c_err( "[rebate_mng](generate_rebate_sql)can't get test2_20002_db" )
        return false
    end

    local test2_30002_db = get_db_by_name( "test2_30002" )
    if not test2_30002_db then
        c_err( "[rebate_mng](generate_rebate_sql)can't get test2_30002_db" )
        return false
    end

    local test1_20001_player_info = {}
    local test1_30001_player_info = {}
    local test2_20002_player_info = {}
    local test2_30002_player_info = {}

    -- 一测应用宝
    generate_recharge_data( test1_20001_db, test1_20001_player_info, "test1_recharge" )
    generate_gs_rank_data( test1_20001_db, test1_20001_player_info, "test1_gs_rank" )
    generate_level_data( test1_20001_db, test1_20001_player_info, "test1_level" )
    generate_login_days_data( test1_20001_db, test1_20001_player_info, "test1_login_days" )

    -- 一测混服
    generate_recharge_data( test1_30001_db, test1_30001_player_info, "test1_recharge" )
    generate_gs_rank_data( test1_30001_db, test1_30001_player_info, "test1_gs_rank" )
    generate_level_data( test1_30001_db, test1_30001_player_info, "test1_level" )
    generate_login_days_data( test1_30001_db, test1_30001_player_info, "test1_login_days" )

    -- 合并一测数据
    local test1_account_info = {}
    merge_player_data( test1_20001_db, test1_20001_player_info, test1_account_info )
    merge_player_data( test1_30001_db, test1_30001_player_info, test1_account_info )

    fix_data( test1_account_info )

    -- 二测应用宝
    generate_recharge_data( test2_20002_db, test2_20002_player_info, "test2_recharge" )
    generate_level_data( test2_20002_db, test2_20002_player_info, "test2_level" )
    generate_login_days_data( test2_20002_db, test2_20002_player_info, "test2_login_days" )

    -- 二测混服
    generate_recharge_data( test2_30002_db, test2_30002_player_info, "test2_recharge" )
    generate_level_data( test2_30002_db, test2_30002_player_info, "test2_level" )
    generate_login_days_data( test2_30002_db, test2_30002_player_info, "test2_login_days" )

    -- 合并二测数据
    local test2_account_info = {}
    merge_player_data( test2_20002_db, test2_20002_player_info, test2_account_info )
    merge_player_data( test2_30002_db, test2_30002_player_info, test2_account_info )

    fix_data( test2_account_info )

    local final_account_info = merge_account_data( test1_account_info, test2_account_info )
    local sql_list = generate_insert_sql( final_account_info )

    insert_final_data( rebate_db, sql_list )
end

