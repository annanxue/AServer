module( "db_login", package.seeall )

ACCOUNT_CACHE_MAX = 10000
PLAYER_CACHE_MAX = 20000

ACCOUNT_PLAYER_NUM_MAX = 5


local sfind, sformat, sgsub, smatch, tinsert, os_time = string.find, string.format, string.gsub, string.match, table.insert, os.time
local job_attr_default = JOBS.JOB_ATTR_DEFAULT
local table_empty = utils.table.is_empty
local ostime = os.time

g_account_cache = g_account_cache or s_cache.new( ACCOUNT_CACHE_MAX )
g_player_cache = g_player_cache or s_cache.new( PLAYER_CACHE_MAX )

-- key为正被锁住的名字；value：锁住该名字的dpid，是否用作随机男名（is_male_random_name_），是否用作随机女名（is_female_random_name_）
g_locked_names = g_locked_names or {}

-- 用于记录正在锁住名字用于创建角色（或改名）的客户端，key为客户端account_id；value：客户端当前锁住的名字
g_account_2_locked_name = g_account_2_locked_name or {}

----------------------------------------------------
--                  sql default
----------------------------------------------------

-- account
DEFAULT_ACCOUNT_COLUMN = 
{
    player_list_ = "return {}",
    last_join_player_id_ = -1,
    authority_ = 0,

    register_ip_ = "",
    recent_logon_ip_ = "",
    recent_logon_time_ = 0,
}

DEFAULT_ACCOUNT_INSERT_SQL = [[
    INSERT INTO ACCOUNT_TBL 
    VALUES 
    ( 
        %d, 
        "return {}",
        -1,
        0,
        0,
        "%s",
        "%s",
        %d
    );
]]

SELECT_ACCOUNT_SQL = [[
    SELECT
        player_list,
        last_join_player_id,
        authority,
        account_first_player_time,
        register_ip,
        recent_logon_ip,
        recent_logon_time
    from ACCOUNT_TBL
    where account_id = %d;
]]


UPDATE_ACCOUNT_SQL = [[
    UPDATE ACCOUNT_TBL SET
        player_list = '%s',
        last_join_player_id = '%d'
    WHERE account_id = %d;
]]

SET_ACCOUNT_AUTHORITY_SQL = [[
    UPDATE ACCOUNT_TBL SET
        authority = %d
    WHERE account_id = '%d';
]]

SET_ACCOUNT_FIRST_PLAYER_TIME_SQL = [[
    UPDATE ACCOUNT_TBL SET
        account_first_player_time = %d
    WHERE account_id = '%d';
]]

SET_ACCOUNT_REGISTER_IP_SQL = [[
    UPDATE ACCOUNT_TBL SET
        register_ip = '%s'
    WHERE account_id = '%d';
]]

UPDATE_ACCOUNT_LOGON_SQL = [[
    UPDATE ACCOUNT_TBL SET
        recent_logon_ip = '%s',
        recent_logon_time = %d
    WHERE account_id = '%d';
]]


DEFAULT_SCENE_ID = channel_mng.CITY_SCENE_ID

-- player
DEFAULT_PLAYER_COLUMN = 
{
    scene_id_ = DEFAULT_SCENE_ID,
    x_ = 0,
    y_ = 0,
    z_ = 0,
    angle_ = 0,
    level_ = 1,
    exp1_ = 0,
    exp2_ = 0,
    gold_ = 0,
    bind_gold_ = 0,
    bind_diamond_ = 0,
    non_bind_diamond_ = 0,
    elam_money_ = 0,
    voucher_ = 0,
    wing_id_ = 0,
    energy_ = player_t.MAX_ENERGY,
    honor_ = 0,
    glory_ = 0,
    total_game_time_ = 0,
    week_game_time_ = 0,
    buff_str_ = "{}",
    ext_param_str_ = "{}",
    favorite_shops_str_ = "",
    learnt_talents_str_ = "",
    title_ = 0, 
    create_time_ = {os_time}, 
    truncheon_id_ = 0,
    last_recover_energy_time_ = os_time(),
    guild_name_ = "", 
    forbid_chat_end_time_ = 0,
    wild_role_ = 1,
    justice_ = 0,
    killer_cd_end_time_ = 0,
    conqueror_cd_end_time_ = 0,
    guild_glory_ = 0,
    guild_contribution_ = 0,
    is_evil_ = 0,
    max_gs_ = 0,
    donate_value_ = 0,
    real_game_time_ = 0,
    soul_power_ = 0,
}

DEFAULT_PLAYER_INSERT_SQL = [[
    INSERT INTO PLAYER_TBL 
    (
        `machine_code_id`,
        `player_name`,
        `account_name`,
        `model_id`,
        `head_icon_id`,
        `scene_id`,
        `x`,
        `y`,
        `z`,
        `angle`,
        `level`,
        `job_id`,
        `skill_slots`,
        `buff`,
        `ext_param`
    )
    VALUES
    (
        %d,
        "%s",
        "%s",
        %d,
        %d,
        %d,
        %f,
        0,
        %f,
        0,
        1,
        %d,
        "%s",
        "%s",
        "%s"
    );
]]

UPDATE_PLAYER_INSERT_SQL = [[
    UPDATE PLAYER_TBL SET
        model_id = ?,
        head_icon_id = ?,
        scene_id = ?,
        x = ?,
        y = ?,
        z = ?,
        angle = ?,
        level = ?,
        job_id = ?,
        exp1 = ?,
        exp2 = ?,
        gold = ?,
        bind_gold = ?,
        bind_diamond = ?,
        non_bind_diamond = ?,
        energy = ?,
        honor = ?,
        glory = ?,
        total_gs = ?,
        last_update_time = now(),
        total_game_time = ?,
        week_game_time = ?,
        buff = ?,
        skill_slots = ?,
        ext_param = ?,
        favorite_shops = ?,
        title = ?,
        truncheon_id = ?, 
        last_recover_energy_time = from_unixtime(?),
        guild_name = ?,
        skill_gs = ?,
        equip_gs = ?,
        god_equip_gs = ?,
        all_transform_gs = ?,
        forbid_chat_end_time = ?,
        wild_role = ?,
        justice = ?,
        killer_cd_end_time = ?,
        conqueror_cd_end_time = ?,
        guild_glory = ?,
        guild_contribution = ?,
        is_evil = ?,
        max_gs = ?,
        donate_value = ?,
        elam_money = ?,
        voucher = ?,
        wing_id = ?,
        mipush_regid = ?,
        real_game_time = ?,
        soul_power = ?
    where player_id = ?;
]]

UPDATE_PLAYER_INSERT_SQL_FORMAT = "iiiffffiiiiiiiiiiiiiissssiiisiiiiiiiiiiiiiiiiisiii"

SELECT_PLAYER_SQL = [[
    SELECT 
    `machine_code_id`,
    `player_name`, 
    `account_name`, 
    `model_id`, 
    `head_icon_id`,
    `scene_id`, 
    `x`, 
    `y`, 
    `z`, 
    `angle`, 
    `level`, 
    `job_id`,
    `exp1`,
    `exp2`,
    `gold`,
    `bind_gold`,
    `bind_diamond`,
    `non_bind_diamond`,
    `energy`,
    `honor`,
    `glory`,
	unix_timestamp(last_update_time), 
    `total_game_time`,
	`week_game_time`,
    `buff`,
    `skill_slots`,
    `ext_param`,
    `favorite_shops`, 
    `total_gs`,
    `title`, 
    unix_timestamp(create_time), 
    `truncheon_id`,
    unix_timestamp(last_recover_energy_time),
    skill_gs, 
    equip_gs,
    god_equip_gs,
    all_transform_gs,
    forbid_chat_end_time,
    wild_role,
    justice,
    killer_cd_end_time,
    conqueror_cd_end_time,
    `guild_glory`,
    `guild_contribution`,
    `is_evil`,
    `max_gs`, 
    `donate_value`, 
    `elam_money`,
    `voucher`,
    `wing_id`,
    `guild_name`,
    `mipush_regid`,
    `real_game_time`,
    `soul_power`
    from PLAYER_TBL where player_id = %d and not deleted;
]]

CHECK_PLAYER_EXIT_SQL = [[
    SELECT 'player_name' from PLAYER_TBL where player_id = ? and not deleted;
]]

CHECK_PLAYER_NAME_SQL = [[
    SELECT 1 from PLAYER_TBL where player_name = ?;
]]
CHECK_PLAYER_NAME_SQL_PARAM_TYPE = "s"
CHECK_PLAYER_NAME_SQL_RESULT_TYPE = "i"

CHECK_DISCARDED_PLAYER_NAME_SQL = [[
    SELECT 1 from DISCARDED_PLAYER_NAME_TBL where player_name = ?;
]]
CHECK_DISCARDED_PLAYER_NAME_SQL_PARAM_TYPE = "s"
CHECK_DISCARDED_PLAYER_NAME_SQL_RESULT_TYPE = "i"

-- 考虑到合服的情况，这里必须用REPLACE而不是INSERT。
-- 合服的情况下A服和B服的DISCARDED_PLAYER_NAME_TBL会合并。可能出现A服的被废弃的角色名正好是
-- B服的正在被使用的角色名。如果用INSERT的话，那么当B服的那个角色改名时就会报数据库插入错误，
-- 改不了名。
REPLACE_DISCARDED_PLAYER_NAME_SQL = [[
    REPLACE INTO DISCARDED_PLAYER_NAME_TBL ( `player_name`, `player_id` ) VALUES ( ?, ? );
]]
REPLACE_DISCARDED_PLAYER_NAME_SQL_PARAM_TYPE = "si"

UPDATE_PLAYER_TBL_ON_PLAYER_RENAME_SQL = [[
    UPDATE PLAYER_TBL SET player_name = ?, bind_diamond = ?, non_bind_diamond = ? where player_id = ?;
]]
UPDATE_PLAYER_TBL_ON_PLAYER_RENAME_SQL_PARAM_TYPE = "siii"

UPDATE_PLAYER_BIND_GOLD_SQL = [[ UPDATE PLAYER_TBL SET bind_gold = %d WHERE player_id = %d; ]]

UPDATE_PLAYER_GOLD_SQL = [[ UPDATE PLAYER_TBL SET gold = %d WHERE player_id = %d; ]]

UPDATE_PLAYER_FORBID_CHAT_END_TIME_SQL = [[ UPDATE PLAYER_TBL SET  forbid_chat_end_time = %d WHERE player_id = %d; ]]

----------------------------------------------------
--                  local
----------------------------------------------------

local function do_check_player_name( _account_id, _player_name )
    if #_player_name == 0 then
        return login_code.PLAYER_NAME_EMPTY 
    end

    -- 检查是否含非法字符
    if smatch( _player_name, "%s" ) ~= nil then
        return login_code.PLAYER_NAME_HAS_INVALID_CHARACTER 
    end

    -- 检查是否所有字符都为数字
    if smatch( _player_name, "[^0-9]" ) == nil then
        return login_code.PLAYER_NAME_ALL_NUMBERS
    end

    -- 检查是否被随机名字锁住
    local locked_name_info = g_locked_names[_player_name]
    if locked_name_info and locked_name_info.account_id_ ~= _account_id then
        return login_code.PLAYER_NAME_UNAVAILABLE
    end
    
    local database = dbserver.g_db_character
    local row_num

    if database:c_stmt_format_query( CHECK_PLAYER_NAME_SQL
                                   , CHECK_PLAYER_NAME_SQL_PARAM_TYPE
                                   , _player_name
                                   , CHECK_PLAYER_NAME_SQL_RESULT_TYPE
                                   ) < 0 then
        c_err( "[db_login](do_check_player_name) CHECK_PLAYER_NAME_SQL sql error, "
             .."_account_id: %d, _player_name: %s", _account_id, _player_name
             )
        return login_code.SQL_ERR
    end
    row_num = database:c_stmt_row_num()
    if row_num ~= 0 then
        return login_code.PLAYER_NAME_UNAVAILABLE
    end

    if database:c_stmt_format_query( CHECK_DISCARDED_PLAYER_NAME_SQL
                                   , CHECK_DISCARDED_PLAYER_NAME_SQL_PARAM_TYPE
                                   , _player_name
                                   , CHECK_DISCARDED_PLAYER_NAME_SQL_RESULT_TYPE
                                   ) < 0 then
        c_err( "[db_login](do_check_player_name) CHECK_PLAYER_NAME_SQL sql error, "
             .."_account_id: %d, _player_name: %s", _account_id, _player_name
             )
        return login_code.SQL_ERR
    end
    row_num = database:c_stmt_row_num()
    if row_num ~= 0 then
        return login_code.PLAYER_NAME_UNAVAILABLE
    end

    return login_code.PLAYER_NAME_OK
end


local function update_account_cache( _account_id, _attr )
    local account = 
    {
        account_name_ = _account_id,
        player_list_ = loadstring( _attr.player_list_ )(),
        last_join_player_id_ = _attr.last_join_player_id_,
        authority_ = _attr.authority_,
        account_first_player_time_ = _attr.account_first_player_time_,
        register_ip_ = _attr.register_ip_,
        recent_logon_ip_ = _attr.recent_logon_ip_,
        recent_logon_time_ = _attr.recent_logon_time_,
    }

    -- 处理need_download字段老数据兼容问题
    for k , player in pairs(account.player_list_) do 
        if not player.need_download then 
            player.need_download = 1            -- 老数据都是可下载的
        end 
    end 

    g_account_cache:put( _account_id, account )
    return account
end

local function update_account_cache_by_database( _account_id, _database )
    local r1, player_list = _database:c_get_str( "player_list", 2048 )
    local r2, last_join_player_id = _database:c_get_int32( "last_join_player_id" )
    local r3, authority = _database:c_get_int32( "authority" )
    local r4, account_first_player_time = _database:c_get_int32( "account_first_player_time" )
    local r5, register_ip = _database:c_get_str( "register_ip", 64 )
    local r6, recent_logon_ip = _database:c_get_str( "recent_logon_ip", 64 )
    local r7, recent_logon_time = _database:c_get_int32( "recent_logon_time")
    if r1 < 0 or r2 < 0 or r3 < 0 or r4 < 0 or r5 < 0 or r6 < 0 or r7 < 0 then
        c_err( "[db_login](update_account_cache_by_database)database get column err!" )
        return { error_ = true }
    end
    local attr = 
    {
        player_list_ = player_list,
        last_join_player_id_ = last_join_player_id,
        authority_ = authority,
        account_first_player_time_ = account_first_player_time,
        register_ip_ = register_ip,
        recent_logon_ip_ = recent_logon_ip,
        recent_logon_time_ = recent_logon_time,
    }
    return update_account_cache( _account_id, attr )
end


local function do_create_account( _account_id, _client_ip_str )

    -- local account = update_account_cache( _account_id, DEFAULT_ACCOUNT_COLUMN )

    local database = dbserver.g_db_character 
    if database:c_modify( sformat( DEFAULT_ACCOUNT_INSERT_SQL, _account_id, _client_ip_str, _client_ip_str, os.time()  ) ) < 0 then
        c_err( "[db_login](do_create_accont)sql exec error!" )
    end
    -- local account = update_account_cache_by_database(_account_id, database)

    set_account_register_ip(_account_id, _client_ip_str)
    update_account_logon(_account_id, _client_ip_str)

    local ret, account = try_update_account_cache(_account_id)

    return account
end


local function update_player_cache( _player_id, _account_name, _msg_list, _attr )
    local player = 
    {
        player_id_ = _player_id,
        account_name_ = _account_name,
    }
    for k, v in pairs( _msg_list ) do
        player[k] = v 
    end
    for k, v in pairs( _attr ) do
        if type(v) == "table" and type(v[1]) == "function" then
            local params = {}
            for i = 2, #v do
                params[i-1] = v[i]
            end
            player[k] = v[1](unpack(params))
        else
            player[k] = v
        end
    end
    g_player_cache:put( _player_id, player )
    return player
end

local function update_player_cache_by_database( _player_id, _database )
    local r1, account_id = _database:c_get_int32( "machine_code_id" )
    local r2, player_name = _database:c_get_str( "player_name", 64 )
    local r3, account_name = _database:c_get_str( "account_name", 32 )
    local r4, model_id = _database:c_get_int32( "model_id" )
    local r5, head_icon_id = _database:c_get_int32( "head_icon_id" )
    local r6, scene_id = _database:c_get_int32( "scene_id" )
    local r7, x = _database:c_get_double( "x" )
    local r8, y = _database:c_get_double( "y" )
    local r9, z = _database:c_get_double( "z" )
    local r10, angle = _database:c_get_double( "angle" )
    local r11, level = _database:c_get_int32( "level" )
    local r12, job_id = _database:c_get_int32( "job_id" )
    local r13, exp1 = _database:c_get_int32( "exp1" )
    local r14, exp2 = _database:c_get_int32( "exp2" )
    local r15, gold = _database:c_get_int32( "gold" )
    local r16, bind_diamond = _database:c_get_int32( "bind_diamond" )
    local r17, non_bind_diamond = _database:c_get_int32( "non_bind_diamond" )
    local r18, energy = _database:c_get_int32( "energy" )
    local r19, honor = _database:c_get_int32( "honor" )
    local r20, last_logout_time = _database:c_get_int32( "unix_timestamp(last_update_time)" )
    local r21, total_game_time = _database:c_get_int32( "total_game_time" )
	local r22, week_game_time = _database:c_get_int32( "week_game_time" )
	local r23, buff = _database:c_get_str( "buff", 512 )
    local r24, skill_slots = _database:c_get_str( "skill_slots", 256 )
    local r25, ext_param = _database:c_get_str( "ext_param", 256 )
    local r26, favorite_shops = _database:c_get_str( "favorite_shops", 128 )
    local r27, glory = _database:c_get_int32( "glory" )
    local r28, total_gs = _database:c_get_int32( "total_gs" )
    local r29, title = _database:c_get_int32( "title" )
    local r30, create_time = _database:c_get_int32( "unix_timestamp(create_time)" )
    local r31, truncheon_id = _database:c_get_int32( "truncheon_id" )
    local r32, last_recover_energy_time = _database:c_get_int32( "unix_timestamp(last_recover_energy_time)" )
    local r33, skill_gs = _database:c_get_int32( "skill_gs" )
    local r34, equip_gs = _database:c_get_int32( "equip_gs" )
    local r35, god_equip_gs = _database:c_get_int32( "god_equip_gs" )
    local r36, all_transform_gs = _database:c_get_int32( "all_transform_gs" )
    local r37, bind_gold = _database:c_get_int32( "bind_gold" )
    local r38, forbid_chat_end_time = _database:c_get_int32( "forbid_chat_end_time" )
    local r39, wild_role = _database:c_get_int32( "wild_role" )
    local r40, justice = _database:c_get_int32( "justice" )
    local r41, killer_cd_end_time = _database:c_get_int32( "killer_cd_end_time" )
    local r42, conqueror_cd_end_time = _database:c_get_int32( "conqueror_cd_end_time" )
    local r43, guild_glory = _database:c_get_int32( "guild_glory" )
    local r44, guild_contribution = _database:c_get_int32( "guild_contribution" )
    local r45, is_evil = _database:c_get_int32( "is_evil" )
    local r46, max_gs = _database:c_get_int32( "max_gs" )
    local r47, donate_value = _database:c_get_int32( "donate_value" )
    local r48, elam_money = _database:c_get_int32( "elam_money" )
    local r49, voucher = _database:c_get_int32( "voucher" )
    local r50, wing_id = _database:c_get_int32( "wing_id" )
    local r51, guild_name =  _database:c_get_str( "guild_name", 100 )
    local r52, mipush_regid = _database:c_get_str( "mipush_regid", 256 )
    local r53, real_game_time = _database:c_get_int32( "real_game_time" )
    local r54, soul_power = _database:c_get_int32( "soul_power" )

    if 
        r1 < 0 or 
        r2 < 0 or 
        r3 < 0 or 
        r4 < 0 or 
        r5 < 0 or 
        r6 < 0 or 
        r7 < 0 or 
        r8 < 0 or 
        r9 < 0 or 
        r10 < 0 or 
        r11 < 0 or 
        r12 < 0 or 
		r13 < 0 or
		r14 < 0 or
        r15 < 0 or
        r16 < 0 or
        r17 < 0 or
        r18 < 0 or
        r19 < 0 or
        r20 < 0 or
        r21 < 0 or
        r22 < 0 or
        r23 < 0 or
        r24 < 0 or
        r25 < 0 or
        r26 < 0 or
        r27 < 0 or
        r28 < 0 or
        r29 < 0 or
        r30 < 0 or
        r31 < 0 or 
        r32 < 0 or
        r33 < 0 or
        r34 < 0 or 
        r35 < 0 or
        r36 < 0 or
        r37 < 0 or
        r38 < 0 or
        r39 < 0 or
        r40 < 0 or
        r41 < 0 or
        r42 < 0 or
        r43 < 0 or
        r44 < 0 or
        r45 < 0 or 
        r46 < 0 or 
        r47 < 0 or 
        r48 < 0 or 
        r49 < 0 or
        r50 < 0 or
        r51 < 0 or 
        r52 < 0 or 
        r53 < 0 or 
        r54 < 0 
    then 
        c_err( "[db_login](update_player_cache_by_database)database get column err!" )
        return nil
    end
    local msg_list = 
    {
        account_id_ = account_id,
        model_id_ = model_id,       
        head_icon_id_ = head_icon_id,
        player_name_ = player_name, 
    }
    local attr =
    {
        scene_id_ = scene_id,
        x_ = x,
        y_ = y,
        z_ = z,
        angle_ = angle,
        level_ = level,
        job_id_ = job_id,
        exp1_ = exp1,
        exp2_ = exp2,
        gold_ = gold,
        bind_gold_ = bind_gold,
        bind_diamond_ = bind_diamond,
        non_bind_diamond_ = non_bind_diamond,
        energy_ = energy,
        honor_ = honor,
        glory_ = glory,
		last_logout_time_ = last_logout_time,
        total_game_time_ = total_game_time,
		week_game_time_ = week_game_time,
        buff_str_ = buff,
        skill_slots_str_ = skill_slots,
        ext_param_str_ = ext_param,
        favorite_shops_str_ = favorite_shops, 
        total_gs_ = total_gs,
        create_time_ = create_time,
        title_ = title, 
        truncheon_id_ = truncheon_id,
        last_recover_energy_time_ = last_recover_energy_time, 
        skill_gs_ = skill_gs,
        equip_gs_ = equip_gs, 
        god_equip_gs_ = god_equip_gs,
        all_transform_gs_ = all_transform_gs,
        forbid_chat_end_time_ = forbid_chat_end_time,
        wild_role_ = wild_role,
        justice_ = justice,
        killer_cd_end_time_ = killer_cd_end_time,
        conqueror_cd_end_time_ = conqueror_cd_end_time,
        guild_glory_ = guild_glory,
        guild_contribution_ = guild_contribution,
        is_evil_ = is_evil,
        max_gs_ = max_gs,
        donate_value_ = donate_value, 
        elam_money_ = elam_money, 
        voucher_ = voucher, 
        wing_id_ = wing_id,
        guild_name_ = guild_name,
        mipush_regid_ = mipush_regid,
        real_game_time_ = real_game_time,
        soul_power_ = soul_power,
    }
    return update_player_cache( _player_id, account_name, msg_list, attr )
end

local function do_save_player_all_system_data( _player )
    local ret_code = 1
    for _, maker in ipairs( player_t.SYSTEM_DATA_MAKER_LIST ) do 
        if db_system_data.do_save_player_system_data( _player, maker ) < 0 then 
            c_err( "[db_login](do_save_player_all_system_data)update %s error! player_id:%d", maker.sql_table_name, _player.player_id_ )
            ret_code = -1
        end 
    end 
    return ret_code
end 

local function save_player_to_db( _player )
    local ret, account = try_update_account_cache( _player.account_id_ )
    if account then
        local player_list = account.player_list_

        local player_id = _player.player_id_
        account.last_join_player_id_ = player_id

        local player = player_list[player_id]
        if not player then
            c_err( "[db_login](save_player_to_db) account_id: %d, player_list does not contain player_id %d", _player.account_id_, player_id )
            return
        end
        player.level_ = _player.level_
        player.model_id_ = _player.model_id_
        player.use_model_ = _player.avatar_.main_fight_
        player.total_gs_ = _player.total_gs_
        player.is_db_new_ = nil
        player.is_player_new_ = _player.is_player_new_
        player.is_account_new_ = _player.is_account_new_
        player.wing_id_ = _player.wing_id_
        player.active_weapon_sfxs_ = _player.active_weapon_sfxs_
        
        do_update_account( _player.account_id_, player_list, player_id )
    else
        c_err( "[db_login](save_player_to_db) Cannot find account of account_id %d, ret code %d.", _player.account_id_, ret )
    end

    local time = c_cpu_ms()

    local database = dbserver.g_db_character 
    if database:c_begin() ~= 0 then
        c_err( "[db_login](save_player_to_db) begin transaction failed, player_id: %d", _player.player_id_ )
        return
    end
    
    if database:c_stmt_format_modify( UPDATE_PLAYER_INSERT_SQL, UPDATE_PLAYER_INSERT_SQL_FORMAT,
        _player.model_id_,
        _player.head_icon_id_,
        _player.scene_id_,
        _player.x_,
        _player.y_,
        _player.z_,
        _player.angle_,
        _player.level_,
        _player.job_id_,
        _player.exp1_,
        _player.exp2_,
        _player.gold_,
        _player.bind_gold_,
        _player.bind_diamond_,
        _player.non_bind_diamond_,
        _player.energy_,
        _player.honor_,
        _player.glory_,
        _player.total_gs_,
        _player.total_game_time_,
        _player.week_game_time_,
        _player.buff_str_,
        _player.skill_slots_str_,
        _player.ext_param_str_,
        _player.favorite_shops_str_,
        _player.title_, 
        _player.truncheon_id_,
        _player.last_recover_energy_time_,
        _player.guild_name_,
        _player.skill_gs_, 
        _player.equip_gs_, 
        _player.god_equip_gs_, 
        _player.all_transform_gs_, 
        _player.forbid_chat_end_time_,
        _player.wild_role_,
        _player.justice_,
        _player.killer_cd_end_time_,
        _player.conqueror_cd_end_time_,
        _player.guild_glory_,
        _player.guild_contribution_,
        _player.is_evil_,
        _player.max_gs_,
        _player.donate_value_, 
        _player.elam_money_,
        _player.voucher_,
        _player.wing_id_,
        _player.mipush_regid_,
        _player.real_game_time_,
        _player.soul_power_,
        _player.player_id_ ) < 0 then
        c_err( "[db_login](save_player_to_db)update db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return 
    end

    if db_bag.do_save_player_bag( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update bag db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_bag.do_save_player_warehouse( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update warehouse db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_skill.do_save_player_skill_talent( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update skill db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_element.do_save_player_element( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update element db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_rune.do_save_player_rune( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update rune db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end
    
    if db_quest.do_save_player_quest( _player ) < 0 then 
         c_err( "[db_login](save_player_to_db)update quest db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end
   
    if db_avatar.do_save_player_avatar( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update avatar db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_soul.do_save_player_soul( _player ) < 0 then
        c_err( "[db_login](save_player_do_db)update soul db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_mastery.do_save_player_mastery( _player ) < 0 then
        c_err( "[db_login](save_player_do_db)update mastery db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_mail.do_save_player_mail( _player ) < 0 then
        c_err("[db_login](save_player_to_db)update mail db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end
    
    if db_pray.do_save_player_pray( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update pray db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end
   
    if db_time.do_save_player_time( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update time db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_newbie_guide.do_save_player_newbie_guide_record( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update newbie guide record db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_mystic.do_save_player_mystic( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update mystic db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if do_save_player_all_system_data( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update system data error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end 
  
    if db_tower.do_save_player_tower( _player ) < 0 then
        c_err( "[db_login](save_player_to_db) update tower db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end
    
    if db_palace.do_save_player_palace( _player ) < 0 then
        c_err( "[db_login](save_player_to_db) update palace db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end    

    if db_daily_goals.do_save_player_daily_goals_record( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update daily goals record db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_attend.do_save_player_attend( _player ) < 0 then
        c_err( "[db_attend](save_player_to_db) update attend db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_rmb.do_save_player_rmb( _player ) < 0 then
       c_err( "[db_login](save_player_to_db)update rmb db error! player_id: %d", _player.player_id_ )
       database:c_rollback()
       return
    end

    if db_snapshot.do_save_player_snapshot( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update snapshot db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_guild.do_save_player_guild( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update guild db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end
    
    if db_friend.do_save_player_friend( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update friend db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_inst.do_save_player_inst( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update inst db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end
    
    if db_truncheon.do_save_player_truncheon( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update truncheon db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_guild_profession.do_save_player_guild_profession( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update guild_profession db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_equip_exchange_shop.do_save_player_equip_exchange_shop_record( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update equip exchange shop record db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_title.do_save_player_title( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update title db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_permanent_attr_record.do_save_player_permanent_attr_record( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update permanent attr record db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_welfare.do_save_player_welfare( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update welfare db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_trade.do_save_player_trade( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update trade db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_vip.do_save_player_vip( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update vip db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_auction.do_save_player_auction( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update auction db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_guild_donation.do_save_player_guild_donation( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update guild_donation db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_guild_pve.do_save_player_guild_pve( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update guild_pve db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_prevent_kill_cheat.do_save_player_prevent_kill_cheat( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update prevent_kill_cheat db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_customer_service.do_save_player_customer_service( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update customer_service db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_god_divination.do_save_player_god_divination( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update god_divination db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_red.do_save_player_red( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update red db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_share.do_save_player_share( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update share db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_level_reward.do_save_player_level_reward( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update level_reward db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_pet.do_save_player_pet( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update pet db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_celebration.do_save_player_celebration( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update celebration db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_holiday_signin_award.do_save_player_holiday_signin_award( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update holiday_signin_award db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_chat.do_save_player_chat( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update chat db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_monthly_signin_award.do_save_player_monthly_signin_award( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update monthly_signin_award db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_mall.do_save_player_mall( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update mall db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_daily_online_award.do_save_player_daily_online_award( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update daily_online_award db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_cycled_recharge_award.do_save_player_cycled_recharge_award( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update cycled_recharge_award db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_growth_fund.do_save_player_growth_fund( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update growth_fund db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_login_activity.do_save_player_login_activity( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update login_activity db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_player_team_ladder.save_player_team_ladder_data_all( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update player team ladder data db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_sanding.do_save_player_sanding( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update sanding db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_survey.do_save_player_survey( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update survey db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_small_data.do_save_player_small_data( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update small_data db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_battle_justice.do_save_player_battle_justice( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update battle_justice db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_hall.do_save_player_hall( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update hall db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_level_recharge.do_save_player_level_recharge( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update level_recharge db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_like.do_save_player_like( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update like db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_carnival.do_save_player_carnival( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update carnival db error! player_id: %d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_one_gold_lucky_draw.do_save_player_one_gold_lucky_draw( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update one_gold_lucky_draw db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_egg.do_save_player_egg( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update egg db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_festival.do_save_player_festival( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update festival db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_player_chat_gift.do_save_player_player_chat_gift( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update player_chat_gift db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_christmas_sell.do_save_player_christmas_sell( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update christmas_sell db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_christmas_recharge_lucky_draw.do_save_player_christmas_recharge_lucky_draw( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update christmas_recharge_lucky_draw db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_wing_system.do_save_player_wing_system( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update wing_system db error! player_id:d", _player.player_id_ )
        database:c_rollback()
        return
    end


    if db_activity.do_save_player_activity( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update activity db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_activity.do_save_player_vip_reward( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update activity db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_activity.do_save_player_resource_magnate( _player ) < 0 then 
        c_err( "[db_login](save_player_to_db)update activity db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_activity.do_save_player_group_buy( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update activity db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_activity.do_save_player_resource_fund( _player ) < 0 then
        c_err( "[db_login](save_player_to_db)update activity db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end

    if db_soulcard.do_save_player_soulcards( _player ) < 0 then
        c_err( "[db_login](db_migrate -> save_player_to_db) update soulcards db error! player_id:%d", _player.player_id_ )
        database:c_rollback()
        return
    end


--$SAVE_DB_DATA

    if database:c_commit() ~= 0 then
        c_err( "[db_login](save_player_to_db) commit transaction failed, player_id: %d", _player.player_id_ )
        return
    end
    time = c_cpu_ms() - time

    c_log( "[db_login](save_player_to_db)update db. player_id:%d, use time: %dms", _player.player_id_, time )
end

function query_player_base( _player_id )
    -- query cache
    local player = g_player_cache:get( _player_id )
    if not player then
        -- load from db
        local database = dbserver.g_db_character 
        if database:c_query( sformat( SELECT_PLAYER_SQL, _player_id )  ) < 0 then
            c_err( "[db_login](query_player_base) SELECT_PLAYER_SQL error, player_id: %d", _player_id )
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        local rows = database:c_row_num()
        if rows ~= 1 then
            c_err( "[db_login](query_player_base)select player row: %d error, player_id:%d", rows , _player_id )
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        if database:c_fetch() == 0 then
            player = update_player_cache_by_database( _player_id, database )
            if not player then
                return login_code.PLAYER_JOIN_SQL_ERR, nil
            end
        else
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
    end
    return login_code.PLAYER_JOIN_OK, player 
end

local function query_player_system_data( _player, _maker )
    local player_id = _player.player_id_
    local table_str = player_t.get_player_system_data( _player, _maker )
    if not table_str then 
        table_str = db_system_data.do_get_player_system_data( player_id, _maker )
        if not table_str then 
            return false
        end
        local table_name = player_t.get_system_data_table_name( _maker ) 
        _player[table_name] = table_str
    end 
    return true
end 

----------------------------------------------------
--                  global
----------------------------------------------------


function do_certify( _account_id, _client_ip_str )
    
    local ret, account = try_update_account_cache( _account_id )
    if ret == login_code.ACC_CACHE_NEW then
        account = do_create_account( _account_id, _client_ip_str )
        return login_code.CERTIFY_OK
    elseif  ret == login_code.ACC_CACHE_FOUND or ret == login_code.ACC_CACHE_UPDATE then

        -- 更新本次登录ip time
        update_account_logon(_account_id, _client_ip_str)
        return login_code.CERTIFY_OK
    elseif ret == login_code.ACC_CACHE_SQLERR then
        c_err( "[db_login](do_certify) sql error, account_id: %d", _account_id )
        return login_code.CERTIFY_SQL_ERR
    end
end

function do_get_account_info( _account_id )
    local account = g_account_cache:get( _account_id )
    if not account then
        c_err( "[db_login](do_get_account_info)no cache account found. account_id: %d", _account_id ) 
        return 0, {}
    end
    local player_list = account.player_list_
    local player_cnt = 0
    local player_array = {}
    for k, v in pairs( player_list ) do
        player_cnt = player_cnt + 1
        player_array[player_cnt] =
        {
            player_id_ = k,
            player_name_ = v.player_name_,
            model_id_ = v.model_id_,
            use_model_ = v.use_model_,
            model_level_ = v.model_level_,
            job_id_ = v.job_id_,
            level_ = v.level_,
            slot_ = v.slot_,
            total_gs_ = v.total_gs_,
            wing_id_ = v.wing_id_ or 0,
            active_weapon_sfxs_ = v.active_weapon_sfxs_,
            need_download = v.need_download ,
        }
    end

    local account_info =
    {
        account_id_ = _account_id,
        account_name_ = account.account_name_,
        player_cnt_ = player_cnt,
        player_list_ = player_array,
        last_join_player_id_ = account.last_join_player_id_,
        authority_ = account.authority_,
        account_first_player_time_ = account.account_first_player_time_,

        register_ip_ = account.register_ip_,
        recent_logon_ip_ = account.recent_logon_ip_,
        recent_logon_time_ = account.recent_logon_time_,

    }

    return account_info
end

function do_check_create_player( _dpid, _account_id, _player_list, _job_id, _player_name )
    -- check player list size
    local num_players = 0
    for k, v in pairs( _player_list ) do
        num_players = num_players + 1
        if ( num_players >= ACCOUNT_PLAYER_NUM_MAX ) then
            return login_code.CREATE_PLAYER_PLAYERLIST_FULL
        end
    end
   
    -- check player job id
    if _job_id < JOBS.JOB_ID_BEGIN or _job_id > JOBS.JOB_ID_END then
		c_err( "[db_login](do_check_create_player) account_id: %d sends invalid job id: %d", _account_id, _job_id ) 
        return login_code.CREATE_PLAYER_JOB_ERR
    end

    -- check player name
    local ret = do_check_player_name( _account_id, _player_name )
    if ret ~= login_code.PLAYER_NAME_OK then
        return ret
    end

    return login_code.CREATE_PLAYER_OK
end

function do_update_account( _account_id, _player_list, _last_join_player_id )
    if dbserver.g_db_character:c_modify( sformat( UPDATE_ACCOUNT_SQL, utils.serialize_table( _player_list ), _last_join_player_id, _account_id ) ) < 0 then
        c_err( "[db_login](do_update_account)sql exec error!" )
        return false
    end
    return true
end

local function init_default_attr_by_job( _msg_list, _job_id )
    -- _job_id在gameserver收到PT_CREATE_PLAYER时应该检查过
    local attr = job_attr_default[_job_id]
    _msg_list.model_id_ = attr.ModelID
    _msg_list.head_icon_id_ = attr.HeadIconID
    _msg_list.total_gs_ = attr.GearScore
    _msg_list.skill_slots_str_, _msg_list.job_skills_str_, _msg_list.unlocked_talents_str_ = player_t.init_default_skill_talent_for_db( _job_id )
    _msg_list.create_time_ = ostime() -- 新建玩家记录一个创建时间
end

function lock_name_for_account( _account_id, _name )
    local locked_name_info = g_locked_names[_name]
    if locked_name_info then
        if locked_name_info.account_id_ ~= _account_id then
            c_err( "[db_login](lock_name_for_account) account %d requests to lock name '%s' which has already been locked by account %d"
                 , _account_id, _name, locked_name_info.account_id_
                 )
        end
        return
    end
    locked_name_info = {}
    locked_name_info.account_id_ = _account_id
    locked_name_info.is_male_random_name_ = db_random_names.is_male_random_name( _name )
    locked_name_info.is_female_random_name_ = db_random_names.is_female_random_name( _name )
    g_locked_names[_name] = locked_name_info
    g_account_2_locked_name[_account_id] = _name
    db_random_names.remove_random_name( _name, false )
end

local function return_locked_random_name( _name, _locked_name_info )
    if _locked_name_info.is_male_random_name_ then
        db_random_names.add_random_name( db_random_names.GENDER_MALE, _name )
    end
    if _locked_name_info.is_female_random_name_ then
        db_random_names.add_random_name( db_random_names.GENDER_FEMALE, _name )
    end
end

function unlock_name_of_account( _account_id )
    local name = g_account_2_locked_name[_account_id]
    if name then
        g_account_2_locked_name[_account_id] = nil
        local locked_name_info = g_locked_names[name]
        return_locked_random_name( name, locked_name_info )
        g_locked_names[name] = nil
    end
end

function unlock_all_names()
    for name, locked_name_info in pairs( g_locked_names ) do
        return_locked_random_name( name, locked_name_info )
    end
    g_account_2_locked_name = {}
    g_locked_names = {}
end

function do_create_player( _dpid, _msg_list )
    local account_id = _msg_list.account_id_
    local player_name = _msg_list.player_name_
    local job_id = _msg_list.job_id_
    local slot = _msg_list.slot_

    init_default_attr_by_job( _msg_list, job_id )
    local model_id = _msg_list.model_id_
    local head_icon_id = _msg_list.head_icon_id_

    -- get account
    local account = g_account_cache:get( account_id )
    if not account then 
        return login_code.CREATE_PLAYER_ACCOUNT_NOT_FOUND
    end

    -- do check create player
    local check_result = do_check_create_player( _dpid, account_id, account.player_list_, job_id, player_name )
    if check_result ~= login_code.CREATE_PLAYER_OK then 
        return check_result
    end
    local account_name = account.account_name_

    -- do delete the player name from random name table
    unlock_name_of_account( account_id )
    if not db_random_names.remove_random_name( player_name, true ) then
        return login_code.SQL_ERR
    end

    -- generate player spawn position
    local spawn_x, spawn_z = channel_mng.get_city_spawn_pos() 

    -- do insert player
    local database = dbserver.g_db_character 
    if database:c_modify( sformat(
        DEFAULT_PLAYER_INSERT_SQL,
        account_id,
        player_name,
        account_name,
        model_id,
        head_icon_id,
        DEFAULT_SCENE_ID,
        spawn_x,
        spawn_z,
        job_id,
        _msg_list.skill_slots_str_,
        DEFAULT_PLAYER_COLUMN.buff_str_,
        DEFAULT_PLAYER_COLUMN.ext_param_str_
        ) ) < 0 then
        return login_code.SQL_ERR
    end

    local player_id = database:c_get_insert_id()

    -- do update cache
    DEFAULT_PLAYER_COLUMN.x_ = spawn_x
    DEFAULT_PLAYER_COLUMN.z_ = spawn_z
    local player = update_player_cache( player_id, account_name, _msg_list, DEFAULT_PLAYER_COLUMN )

    if db_skill.do_save_player_skill_talent( player ) < 0 then
        c_err( "[db_login](do_create_player) do_save_player_skill_talent error, player_id: %d", player_id )
    end

    local player_list = account.player_list_
    local is_account_new = table_empty( player_list ) -- 是否新账号
    player_list[player_id] =
    {
        player_name_ = player_name,
        model_id_ = model_id,
        use_model_ = player.use_model_,
        model_level_ = player.model_level_,
        job_id_ = job_id,
        slot_ = slot,
        level_ = player.level_,
        total_gs_ = player.total_gs_,
        is_db_new_ = true,
        is_account_new_ = is_account_new,
        is_player_new_ = true,
        need_download = 0,

    } 
    -- do save account
    if not do_update_account( account_id, player_list, account.last_join_player_id_ ) then
        return login_code.SQL_ERR
    end
    c_trace( "[db_login](do_create_player)player id: %d", player.player_id_ )
    if is_account_new then
        set_account_first_player_time(account_id, ostime())
    end
    return login_code.CREATE_PLAYER_OK, player_id, player_name
end

function do_delete_player( _account_id, _player_id )
    -- do update player cache
    if g_player_cache:get( _player_id ) then
        g_player_cache:put( _player_id, nil )
    end
    -- get account
    local account = g_account_cache:get( _account_id )
    if not account then 
        return login_code.DELETE_PLAYER_ACCOUNT_NOT_FOUND
    end
    -- do update account cache
    local player_list = account.player_list_
    if player_list[_player_id] == nil then
        return login_code.DELETE_PLAYER_PLAYER_NOT_FOUND
    end
    player_list[_player_id] = nil
    -- do save account
    if not do_update_account( _account_id, player_list, account.last_join_player_id_ ) then
        return login_code.DELETE_PLAYER_SQL_ERR
    end
    -- do delete player
    if dbserver.g_db_character:c_modify( sformat( "UPDATE PLAYER_TBL set deleted = true where player_id = %d", _player_id ) ) < 0 then
        c_err( "[db_login](do_delete_player)sql exec error!" )
        return login_code.DELETE_PLAYER_SQL_ERR
    end
    c_log( "[db_login](do_delete_player)player id: %d", _player_id ) 
	return login_code.DELETE_PLAYER_OK
end

function do_player_join( _player_id, _log_info )
    -- query base
    local retcode, player = query_player_base( _player_id )
    if not player then
        return retcode, nil
    end

    -- query account
    local ret, account = try_update_account_cache( player.account_id_ )
    if not account then
        c_err( "[db_login](do_player_join) player_id:%d, player.account_id_:%d; cannot find the account", _player_id, player.account_id_ ) 
        return login_code.PLAYER_JOIN_ACCOUNT_NOT_FOUND, nil
    end

    -- whether the player has ever joined the game
    player.is_db_new_ = account.player_list_[_player_id].is_db_new_
    player.is_account_new_ = account.player_list_[_player_id].is_account_new_
    player.is_player_new_ = account.player_list_[_player_id].is_player_new_

    -- query bag
    local bag = player.bag_
    if not bag then
        bag = db_bag.do_get_player_bag( _player_id )
        if not bag then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.bag_ = bag
    end

    -- query warehouse
    local warehouse = player.warehouse_
    if not warehouse then
        warehouse = db_bag.do_get_player_warehouse( _player_id )
        if not warehouse then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.warehouse_ = warehouse
    end

    -- query skill
    local job_skills_str = player.job_skills_str_
    local learnt_talents_str = player.learnt_talents_str_
    local unlocked_talents_str = player.unlocked_talents_str_
    if not job_skills_str or not learnt_talents_str or not unlocked_talents_str then
        job_skills_str, learnt_talents_str, unlocked_talents_str = db_skill.do_get_player_skill_talent( _player_id )
        if not job_skills_str or not learnt_talents_str or not unlocked_talents_str then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.job_skills_str_ = job_skills_str
        player.learnt_talents_str_ = learnt_talents_str
        player.unlocked_talents_str_ = unlocked_talents_str
    end

    -- query element
    local pvp_element = player.pvp_element_
    local pve_element = player.pve_element_
    local master_skills_str = player.master_skills_str_
    local practice_skills_str = player.practice_skills_str_
    if not pvp_element or not pve_element or not master_skills_str or not practice_skills_str then
        pvp_element, pve_element, master_skills_str, practice_skills_str = db_element.do_get_player_element( _player_id )
        if not pvp_element or not pve_element or not master_skills_str or not practice_skills_str then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.pvp_element_ = pvp_element
        player.pve_element_ = pve_element
        player.master_skills_str_ = master_skills_str
        player.practice_skills_str_ = practice_skills_str
    end

    -- query player rune
    local rune = player.rune_
    if not rune then
        rune = db_rune.do_get_player_rune( _player_id )
        if not rune then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.rune_ = rune
    end
    
    --query quest
    local quest = player.quest_
    if not quest then 
        quest = db_quest.do_get_player_quest( _player_id )
        if not quest then 
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.quest_ = quest
    end 

    -- query avatar 
    local avatar = player.avatar_
    if not avatar then
        avatar = db_avatar.do_get_player_avatar( _player_id )
        if not avatar then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.avatar_ = avatar
    end

    -- query soul
    local soul = player.soul_
    if not soul then
        soul = db_soul.do_get_player_soul( _player_id )
        if not soul then 
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.soul_ = soul
    end

    -- query mastery
    local mastery = player.mastery_
    if not mastery then
        mastery = db_mastery.do_get_player_mastery( _player_id )
        if not mastery then 
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.mastery_ = mastery
    end

    -- query mail
    local mail_box = player.mail_box_
    if not mail_box then
        mail_box = db_mail.do_get_player_mails( _player_id )
        if not mail_box then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.mail_box_ = mail_box
    end

    -- query unread global mail
    player.unread_gm_global_mails_ = db_mail.do_get_unread_gm_global_mails( mail_box.gm_global_mail_id_read_ )

    -- query pray 
    local pray_str = player.pray_str_
    if not pray_str then
        pray_str = db_pray.do_get_player_pray( _player_id )
        if not pray_str then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.pray_str_ = pray_str
    end

    -- query time
    local time = player.time_
    if not time then
        time = db_time.do_get_player_time( _player_id )
        if not time then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.time_ = time
    end

    -- query newbie guide record
    local newbie_guide_record_str = player.newbie_guide_record_str_
    if not newbie_guide_record_str then
        newbie_guide_record_str = db_newbie_guide.do_get_player_newbie_guide_record( _player_id )
        if not newbie_guide_record_str then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.newbie_guide_record_str_ = newbie_guide_record_str
    end

   -- query mystic
    local mystic_str = player.mystic_str_
    if not mystic_str then
        mystic_str = db_mystic.do_get_player_mystic( _player_id )
        if not mystic_str then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.mystic_str_ = mystic_str
    end

    --query tower data
    local tower_data = player.tower_data_
    if not tower_data then
        tower_data = db_tower.do_get_player_tower( _player_id )
        if not tower_data then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.tower_data_ = tower_data
    end

    --query palace data
    local palace_data = player.palace_data_
    if not palace_data then
        palace_data = db_palace.do_get_player_palace( _player_id )
        if not palace_data then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.palace_data_ = palace_data
    end

    -- query shops
    local shops = player.shops_
    if not shop then
        shops = db_shop.do_get_player_shops( _player_id )
        if not shops then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.shops_ = shops
    end
    
    -- query gold_exchange
    local gold_exchanges_str = player.gold_exchanges_str_
    if not gold_exchanges_str then
        gold_exchanges_str = db_gold_exchange.do_get_player_gold_exchanges( _player_id )
        if not gold_exchanges_str then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.gold_exchanges_str_ = gold_exchanges_str
    end

    --query system data
    for _, maker in ipairs( player_t.SYSTEM_DATA_MAKER_LIST ) do 
        local is_ok = query_player_system_data( player, maker )
        if not is_ok then 
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end 
    end 
    
    --query daily goals
    local daily_goals_ = player.daily_goals_
    if not daily_goals_ then
        daily_goals_ = db_daily_goals.do_get_player_daily_goals_record( _player_id )
        if not daily_goals_ then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.daily_goals_ = daily_goals_
    end

    -- query attend data
    local attend_str = player.attend_str_
    if not attend_str then
        attend_str = db_attend.do_get_player_attend( _player_id )
        if not attend_str then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.attend_str_ = attend_str
    end

    -- query player rmb
    local rmb = player.rmb_
    if not rmb then
        rmb = db_rmb.do_get_player_rmb( _player_id )
        if not rmb then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.rmb_ = rmb
    end

    local snapshot = player.snapshot_ 
    if not snapshot then
        snapshot = db_snapshot.do_get_player_snapshot( _player_id )
        if not snapshot then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.snapshot_ = snapshot
    end 

    local guild = player.guild_ 
    if not guild then
        guild = db_guild.do_get_player_guild( _player_id )
        if not guild then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.guild_ = guild
    end 

    local friend = player.friend_ 
    if not friend then
        friend = db_friend.do_get_player_friend( _player_id )
        if not friend then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.friend_ = friend
    end 

    player.log_info_ = _log_info

    local inst = player.inst_ 
    if not inst then
        inst = db_inst.do_get_player_inst( _player_id )
        if not inst then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.inst_ = inst
    end 

    local truncheon = player.truncheon_ 
    if not truncheon then
        truncheon = db_truncheon.do_get_player_truncheon( _player_id )
        if not truncheon then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.truncheon_ = truncheon
    end 

    local guild_profession = player.guild_profession_ 
    if not guild_profession then
        guild_profession = db_guild_profession.do_get_player_guild_profession( _player_id )
        if not guild_profession then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.guild_profession_ = guild_profession
    end 

    -- query equip exchange shop record
    local record_str_tbl = player.equip_exchange_shop_rec_str_tbl_
    if not record_str_tbl then
        record_str_tbl = db_equip_exchange_shop.do_get_player_equip_exchange_shop_record( _player_id )
        if not record_str_tbl then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.equip_exchange_shop_rec_str_tbl_ = record_str_tbl
    end

    local title_data = player.title_data_ 
    if not title_data then
        title_data = db_title.do_get_player_title( _player_id )
        if not title_data then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.title_data_ = title_data
    end 

    -- query permanent attr record
    local permanent_attr_record_str = player.permanent_attr_record_str_
    if not permanent_attr_record_str then
        permanent_attr_record_str = db_permanent_attr_record.do_get_player_permanent_attr_record( _player_id )
        if not permanent_attr_record_str then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.permanent_attr_record_str_ = permanent_attr_record_str
    end

    local welfare = player.welfare_ 
    if not welfare then
        welfare = db_welfare.do_get_player_welfare( _player_id )
        if not welfare then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.welfare_ = welfare
    end 

    local trade = player.trade_ 
    if not trade then
        trade = db_trade.do_get_player_trade( _player_id )
        if not trade then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.trade_ = trade
    end 

    local vip = player.vip_ 
    if not vip then
        vip = db_vip.do_get_player_vip( _player_id )
        if not vip then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.vip_ = vip
    end 

    local auction = player.auction_ 
    if not auction then
        auction = db_auction.do_get_player_auction( _player_id )
        if not auction then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.auction_ = auction
    else
        auction.item_list_ = db_auction.get_player_auction_items( _player_id )
    end 

    local guild_donation = player.guild_donation_ 
    if not guild_donation then
        guild_donation = db_guild_donation.do_get_player_guild_donation( _player_id )
        if not guild_donation then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.guild_donation_ = guild_donation
    end 

    local guild_pve = player.guild_pve_ 
    if not guild_pve then
        guild_pve = db_guild_pve.do_get_player_guild_pve( _player_id )
        if not guild_pve then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.guild_pve_ = guild_pve
    end

    local prevent_kill_cheat = player.prevent_kill_cheat_
    if not prevent_kill_cheat then
        prevent_kill_cheat = db_prevent_kill_cheat.do_get_player_prevent_kill_cheat( _player_id )
        if not prevent_kill_cheat then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.prevent_kill_cheat_ = prevent_kill_cheat
    end

    local customer_service = player.customer_service_ 
    if not customer_service then
        customer_service = db_customer_service.do_get_player_customer_service( _player_id )
        if not customer_service then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.customer_service_ = customer_service
    end 

    local god_divination = player.god_divination_ 
    if not god_divination then
        god_divination = db_god_divination.do_get_player_god_divination( _player_id )
        if not god_divination then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.god_divination_ = god_divination
    end 

    local red = player.red_ 
    if not red then
        red = db_red.do_get_player_red( _player_id )
        if not red then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.red_ = red
    end 

    local chat = player.chat_ 
    if not chat then
        chat = db_chat.do_get_player_chat( _player_id )
        if not chat then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.chat_ = chat
    end 

    local level_reward = player.level_reward_ 
    if not level_reward then
        level_reward = db_level_reward.do_get_player_level_reward( _player_id )
        if not level_reward then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.level_reward_ = level_reward
    end 

    local pet = player.pet_ 
    if not pet then
        pet = db_pet.do_get_player_pet( _player_id )
        if not pet then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.pet_ = pet
    end 

    local celebration = player.celebration_ 
    if not celebration then
        celebration = db_celebration.do_get_player_celebration( _player_id )
        if not celebration then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.celebration_ = celebration
    end 

    local holiday_signin_award = player.holiday_signin_award_ 
    if not holiday_signin_award then
        holiday_signin_award = db_holiday_signin_award.do_get_player_holiday_signin_award( _player_id )
        if not holiday_signin_award then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.holiday_signin_award_ = holiday_signin_award
    end 

    local monthly_signin_award = player.monthly_signin_award_ 
    if not monthly_signin_award then
        monthly_signin_award = db_monthly_signin_award.do_get_player_monthly_signin_award( _player_id )
        if not monthly_signin_award then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.monthly_signin_award_ = monthly_signin_award
    end 

    local mall = player.mall_ 
    if not mall then
        mall = db_mall.do_get_player_mall( _player_id )
        if not mall then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.mall_ = mall
    end 

    local daily_online_award = player.daily_online_award_ 
    if not daily_online_award then
        daily_online_award = db_daily_online_award.do_get_player_daily_online_award( _player_id )
        if not daily_online_award then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.daily_online_award_ = daily_online_award
    end 

    local cycled_recharge_award = player.cycled_recharge_award_ 
    if not cycled_recharge_award then
        cycled_recharge_award = db_cycled_recharge_award.do_get_player_cycled_recharge_award( _player_id )
        if not cycled_recharge_award then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.cycled_recharge_award_ = cycled_recharge_award
    end 

    local growth_fund = player.growth_fund_ 
    if not growth_fund then
        growth_fund = db_growth_fund.do_get_player_growth_fund( _player_id )
        if not growth_fund then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.growth_fund_ = growth_fund
    end 

    local login_activity = player.login_activity_ 
    if not login_activity then
        login_activity = db_login_activity.do_get_player_login_activity( _player_id )
        if not login_activity then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.login_activity_ = login_activity
    end 

    -- query team ladder data 
    local team_ladder_data = player.team_ladder_db_data_
    if not team_ladder_data then
        team_ladder_data = db_player_team_ladder.get_player_team_ladder_data( _player_id )
        if not team_ladder_data then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.team_ladder_db_data_ = team_ladder_data
    end

    local sanding = player.sanding_ 
    if not sanding then
        sanding = db_sanding.do_get_player_sanding( _player_id )
        if not sanding then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.sanding_ = sanding
    end 

    local survey = player.survey_ 
    if not survey then
        survey = db_survey.do_get_player_survey( _player_id )
        if not survey then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.survey_ = survey
    end 

    local small_data = player.small_data_ 
    if not small_data then
        small_data = db_small_data.do_get_player_small_data( _player_id )
        if not small_data then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.small_data_ = small_data
    end 

    local battle_justice = player.battle_justice_ 
    if not battle_justice then
        battle_justice = db_battle_justice.do_get_player_battle_justice( _player_id )
        if not battle_justice then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.battle_justice_ = battle_justice
    end 

    local hall = player.hall_ 
    if not hall then
        hall = db_hall.do_get_player_hall( _player_id )
        if not hall then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.hall_ = hall
    end 

    local level_recharge = player.level_recharge_ 
    if not level_recharge then
        level_recharge = db_level_recharge.do_get_player_level_recharge( _player_id )
        if not level_recharge then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.level_recharge_ = level_recharge
    end 

    -- query like
    local like = player.like_
    if not like then
        like = db_like.do_get_player_like( _player_id )
        if not like then 
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.like_ = like
    end

    -- query carnival
    local carnival = player.carnival_
    if not carnival then
        carnival = db_carnival.do_get_player_carnival( _player_id )
        if not carnival then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.carnival_ = carnival
    end

    local one_gold_lucky_draw = player.one_gold_lucky_draw_ 
    if not one_gold_lucky_draw then
        one_gold_lucky_draw = db_one_gold_lucky_draw.do_get_player_one_gold_lucky_draw( _player_id )
        if not one_gold_lucky_draw then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.one_gold_lucky_draw_ = one_gold_lucky_draw
    end 

    local egg = player.egg_ 
    if not egg then
        egg = db_egg.do_get_player_egg( _player_id )
        if not egg then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.egg_ = egg
    end 

    local festival = player.festival_ 
    if not festival then
        festival = db_festival.do_get_player_festival( _player_id )
        if not festival then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.festival_ = festival
    end 

    local player_chat_gift = player.player_chat_gift_ 
    if not player_chat_gift then
        player_chat_gift = db_player_chat_gift.do_get_player_player_chat_gift( _player_id )
        if not player_chat_gift then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.player_chat_gift_ = player_chat_gift
    end 

    local christmas_sell = player.christmas_sell_ 
    if not christmas_sell then
        christmas_sell = db_christmas_sell.do_get_player_christmas_sell( _player_id )
        if not christmas_sell then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.christmas_sell_ = christmas_sell
    end 

    local christmas_recharge_lucky_draw = player.christmas_recharge_lucky_draw_ 
    if not christmas_recharge_lucky_draw then
        christmas_recharge_lucky_draw = db_christmas_recharge_lucky_draw.do_get_player_christmas_recharge_lucky_draw( _player_id )
        if not christmas_recharge_lucky_draw then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.christmas_recharge_lucky_draw_ = christmas_recharge_lucky_draw
    end 

    local wing_system = player.wing_system_ 
    if not wing_system then
        wing_system = db_wing_system.do_get_player_wing_system( _player_id )
        if not wing_system then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.wing_system_ = wing_system
    end 

    local is_activity_need_load = false
    for activity_type, param_config in ipairs(activity_mng.ACTIVITY_MAP) do
        if not player[param_config.name_.."_"] then
            is_activity_need_load = true
            break
        end
    end

    local activity_str_tbl = db_activity.do_get_player_activity(_player_id)
    if not activity_str_tbl or #activity_str_tbl ~= #activity_mng.ACTIVITY_MAP then
        return login_code.PLAYER_JOIN_SQL_ERR , nil
    end

    for activity_type, param_config in ipairs(activity_mng.ACTIVITY_MAP) do
        player[param_config.name_.."_"] = activity_str_tbl[activity_type]
    end

    if not player.vip_reward_ then
       local vip_reward = db_activity.do_get_player_vip_reward(_player_id)
       if not vip_reward then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
       end
       player.vip_reward_ = vip_reward
    end

    if not player.resource_magnate_ then
       local resource_magnate = db_activity.do_get_player_resource_magnate(_player_id)
       if not resource_magnate then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
       end
       player.resource_magnate_ = resource_magnate
    end

    if not player.group_buy_ then
        local group_buy = db_activity.do_get_player_group_buy(_player_id)
        if not group_buy then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.group_buy_ = group_buy
    end

    if not player.resource_fund_ then
        local resource_fund = db_activity.do_get_player_resource_fund(_player_id)
        if not resource_fund then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.resource_fund_ = resource_fund
    end

    --query soulcard data
    if not player.soulcard_data_ then
        local soulcard_data = db_soulcard.do_get_player_soulcards( _player_id )
        if not soulcard_data then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.soulcard_data_ = soulcard_data
    end


--$GET_DB_DATA

    -- query other......
    return login_code.PLAYER_JOIN_OK, player 
end

function do_save_player( _online, _player )
    c_log( "[db_login](do_save_player) online: %d, player_id: %d", _online and 1 or 0, _player.player_id_ )
    _player.last_logout_time_ = os.time()
    g_player_cache:put( _player.player_id_, _player )
    save_player_to_db( _player )
end

function try_update_account_cache( _account_id )
    -- query cache
    local account = g_account_cache:get( _account_id )
    if account then
        return login_code.ACC_CACHE_FOUND, account
    end
    local database = dbserver.g_db_character 
    -- query db
    if database:c_query( sformat( SELECT_ACCOUNT_SQL, _account_id ) ) < 0 then
        c_err( "[db_login](try_update_account_cache) sql error, account_id: %d", _account_id )
        return login_code.ACC_CACHE_SQLERR, nil
    end
    local rows = database:c_row_num()
    -- do create
    if rows == 0 then
        return login_code.ACC_CACHE_NEW, nil
    end
    -- do fetch
    if database:c_fetch() == 0 then
        local account = update_account_cache_by_database( _account_id, database )
        return login_code.ACC_CACHE_UPDATE, account
    else 
        c_err( "[db_login](try_update_account_cache) sql error, account_id: %d", _account_id )
        return login_code.ACC_CACHE_SQLERR, nil
    end

    return login_code.ACC_CACHE_UNKNOWN, nil
end

function set_account_authority( _account_id, _authority )
    local account = g_account_cache:get( _account_id )
    if account then
        account.authority_ = _authority
    end

    if dbserver.g_db_character:c_modify( sformat( SET_ACCOUNT_AUTHORITY_SQL, _authority, _account_id ) ) < 0 then
        c_err( "[db_login](set_account_authority) _account_name: %s, _authority: %d; update sql error", _account_name, _authority )
    end
end

function set_account_first_player_time( _account_id, _time )
    local account = g_account_cache:get( _account_id )
    if account then
        account.account_first_player_time_  = _time
    end

    if dbserver.g_db_character:c_modify( sformat( SET_ACCOUNT_FIRST_PLAYER_TIME_SQL, _time, _account_id ) ) < 0 then
        c_err( "[db_login](set_account_first_player_time) _account_name: %s, _time: %d; update sql error", _account_name, _time )
    end
end

function set_account_register_ip(_account_id, _client_ip_str)
    local account = g_account_cache:get( _account_id )
    if account then
        account.register_ip_  = _client_ip_str
    end

    if dbserver.g_db_character:c_modify( sformat( SET_ACCOUNT_REGISTER_IP_SQL, _client_ip_str, _account_id ) ) < 0 then
        c_err( "[db_login](set_account_first_player_time) _account_id: %d; update sql error", _account_id )
    end
end

function update_account_logon(_account_id, _client_ip_str)
    local account = g_account_cache:get( _account_id )
    if account then
        account.recent_logon_ip_  = _client_ip_str
        account.recent_logon_time_ = os.time()
    end

    if dbserver.g_db_character:c_modify( sformat( UPDATE_ACCOUNT_LOGON_SQL, _client_ip_str, os.time(), _account_id ) ) < 0 then
        c_err( "[db_login](update_account_logon) _account_id: %d; update sql error", _account_id )
    end
end


function is_valid_player( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( CHECK_PLAYER_EXIT_SQL, "i", _player_id, "s" ) < 0 then
        c_err( "[db_login](is_valid_player) c_stmt_format_query on player_name failed, player id: %d", _player_id )
        return false
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return false
    end

    return true
end

function update_player_bind_gold( _player_id, _bind_gold )
    local player = g_player_cache:get( _player_id )
    if player then
        player.bind_gold_ = _bind_gold
    end
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( UPDATE_PLAYER_BIND_GOLD_SQL, _bind_gold, _player_id ) ) < 0 then
        c_err( "[db_login](update_player_bind_gold) UPDATE_PLAYER_BIND_GOLD_SQL error, player_id: %d, gold: %d", _player_id, _bind_gold )
        return
    end
    c_log( "[db_login](update_player_bind_gold) player bind_gold updated, player_id: %d, bind_gold: %d", _player_id, _bind_gold )
end

function update_player_gold( _player_id, _gold )
    local player = g_player_cache:get( _player_id )
    if player then
        player.gold_ = _gold
    end
    local database = dbserver.g_db_character 
    if database:c_modify( sformat( UPDATE_PLAYER_GOLD_SQL, _gold, _player_id ) ) < 0 then
        c_err( "[db_login](update_player_gold) UPDATE_PLAYER_GOLD_SQL error, player_id: %d, gold: %d", _player_id, _gold )
        return
    end
    c_log( "[db_login](update_player_gold) player gold updated, player_id: %d, gold: %d", _player_id, _gold )
end

function update_player_need_download( account_id , player_id   )
    local ret, account = try_update_account_cache( account_id )
    if account then 
        local player_list = account.player_list_
        local player = player_list[player_id]
        player.need_download = 1
    end 
end 


function update_player_forbid_end_time( _player_id, _end_time )
    local player = g_player_cache:get( _player_id )
    if player then
        player.forbid_chat_end_time_ = _end_time
    end

    local database = dbserver.g_db_character
    if database:c_modify( sformat( UPDATE_PLAYER_FORBID_CHAT_END_TIME_SQL,  _end_time, _player_id ) ) < 0 then
        c_err( "[db_login](update_player_forbid_end_time) UPDATE_PLAYER_FORBID_CHAT_END_TIME_SQL error, player_id: %d, _end_time: %d", _player_id, _end_time )
        return false
    end
    c_log( "[db_login](update_player_forbid_end_time) player forbid chat end time updated, player_id: %d, end_time: %d", _player_id,  _end_time )
end

function do_player_rename_check( _account_id, _player_id, _player_name )
    local ret = do_check_player_name( _account_id, _player_name )
    dbserver.remote_call_game( "dbclient.on_player_rename_check_result", _player_id, _player_name, ret )
    if ret == login_code.PLAYER_NAME_OK then
        lock_name_for_account( _account_id, _player_name )
    end
    c_log( "[db_login](do_player_rename_check) account_id: %d, player_id: %d, player_name: %s, result: %d"
         , _account_id, _player_id, _player_name, ret
         )
end

function do_player_rename( _account_id, _player_id, _player_name, _bind_diamond, _non_bind_diamond )
    -- do delete the player name from random name table
    unlock_name_of_account( _account_id )
    if not db_random_names.remove_random_name( _player_name, true ) then
        dbserver.remote_call_game( "dbclient.on_player_rename_result", _player_id, _player_name, login_code.SQL_ERR )
        c_err( "[db_login](do_player_rename) failed to remove name from random name tables, "
             .."player_name: %s, player_id: %d"
             , _player_name, _player_id
             )
        return
    end

    local account_ret_code, account = try_update_account_cache( _account_id )
    if not account then
        dbserver.remote_call_game( "dbclient.on_player_rename_result", _player_id, _player_name, login_code.SQL_ERR )
        c_err( "[db_login](do_player_rename) try_update_account_cache returned account nil, "
             .."ret_code: %d, account_id: %d, player_id: %d"
             , account_ret_code, _account_id, _player_id
             )
        return
    end

    local player_list = account.player_list_
    local player = player_list[_player_id]
    local old_player_name = player.player_name_
    player.player_name_ = _player_name
    if not do_update_account( _account_id, player_list, account.last_join_player_id_ ) then
        dbserver.remote_call_game( "dbclient.on_player_rename_result", _player_id, _player_name, login_code.SQL_ERR )
        c_err( "[db_login](do_player_rename) do_update_account failed, account_id: %d, player_id: %d", _account_id, _player_id )
        return
    end

    local database = dbserver.g_db_character 

    if database:c_stmt_format_modify( REPLACE_DISCARDED_PLAYER_NAME_SQL
                                    , REPLACE_DISCARDED_PLAYER_NAME_SQL_PARAM_TYPE
                                    , old_player_name
                                    , _player_id
                                    ) < 0 then
        dbserver.remote_call_game( "dbclient.on_player_rename_result", _player_id, _player_name, login_code.SQL_ERR )
        c_err( "[db_login](do_player_rename) REPLACE_DISCARDED_PLAYER_NAME_SQL error, "
             .."player_id: %d, old player_name: %s, new player_name: %s, remained bind_diamond: %d, remained non_bind_diamond: %d"
             , _player_id, old_player_name, _player_name, _bind_diamond, _non_bind_diamond )
        return
    end

    if database:c_stmt_format_modify( UPDATE_PLAYER_TBL_ON_PLAYER_RENAME_SQL
                                    , UPDATE_PLAYER_TBL_ON_PLAYER_RENAME_SQL_PARAM_TYPE
                                    , _player_name
                                    , _bind_diamond
                                    , _non_bind_diamond
                                    , _player_id
                                    ) < 0 then
        dbserver.remote_call_game( "dbclient.on_player_rename_result", _player_id, _player_name, login_code.SQL_ERR )
        c_err( "[db_login](do_player_rename) UPDATE_PLAYER_TBL_ON_PLAYER_RENAME_SQL error, "
             .."player_id: %d, old player_name: %s, new player_name: %s, remained bind_diamond: %d, remained non_bind_diamond: %d"
             , _player_id, old_player_name, _player_name, _bind_diamond, _non_bind_diamond
             )
        return
    end

    dbserver.remote_call_game( "dbclient.on_player_rename_result", _player_id, _player_name, login_code.PLAYER_NAME_OK )
    c_log( "[db_login](do_player_rename) player renamed, "
         .."player_id: %d, old player_name: %s, new player_name: %s, remained bind_diamond: %d, remained non_bind_diamond: %d"
         , _player_id, old_player_name, _player_name, _bind_diamond, _non_bind_diamond
         )
end

function get_player_snapshot(_player_id)
    local player = g_player_cache:get( _player_id )
    if not player then
        -- load from db
        local database = dbserver.g_db_character 
        if database:c_query( sformat( SELECT_PLAYER_SQL, _player_id )  ) < 0 then
            c_err( "[db_login](get_player_snapshot) SELECT_PLAYER_SQL error, player_id: %d", _player_id )
            return 
        end
        local rows = database:c_row_num()
        if rows ~= 1 then
            c_err( "[db_login](get_player_snapshot)select player row: %d error, player_id:%d", rows , _player_id )
            return 
        end 
        if database:c_fetch() == 0 then
            player = update_player_cache_by_database( _player_id, database )
            if not player then
                return
            end
        else
            return 
        end
    end

    -- query bag
    local bag = player.bag_
    if not bag then
        bag = db_bag.do_get_player_bag( _player_id )
        if not bag then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.bag_ = bag
    end

    -- query avatar
    local avatar = player.avatar_
    if not avatar then
        avatar = db_avatar.do_get_player_avatar( _player_id )
        if not avatar then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.avatar_ = avatar
    end

    -- query soul
    local soul = player.soul_
    if not soul then
        soul = db_soul.do_get_player_soul( _player_id )
        if not soul then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.soul_ = soul
    end

    -- wing_system_
    local wing_system = player.wing_system_
    if not wing_system then
        wing_system = db_wing_system.do_get_player_wing_system( _player_id )
        if not wing_system then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.wing_system_ = wing_system
    end

    -- query truncheon
    local truncheon = player.truncheon_ 
    if not truncheon then
        truncheon = db_truncheon.do_get_player_truncheon( _player_id )
        if not truncheon then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.truncheon_ = truncheon
    end 

    -- query guild profession 
    local guild_profession = player.guild_profession_ 
    if not guild_profession then
        guild_profession = db_guild_profession.do_get_player_guild_profession( _player_id )
        if not guild_profession then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.guild_profession_ = guild_profession
    end 

    local guild = player.guild_ 
    if not guild then
        guild = db_guild.do_get_player_guild( _player_id )
        if not guild then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.guild_ = guild
    end 

    local snapshot = player.snapshot_ 
    if not snapshot then
        snapshot = db_snapshot.do_get_player_snapshot( _player_id )
        if not snapshot then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.snapshot_ = snapshot
    end

    local rmb = player.rmb_
    if not rmb then
        rmb = db_rmb.do_get_player_rmb( _player_id )
        if not rmb then
            return login_code.PLAYER_JOIN_SQL_ERR, nil
        end
        player.rmb_ = rmb
    end
    
    local vip = player.vip_ 
    if not vip then
        vip = db_vip.do_get_player_vip( _player_id )
        if not vip then
            return login_code.PLAYER_JOIN_SQL_ERR , nil
        end
        player.vip_ = vip
    end 


    return player
end

function batch_get_player_snapshot(_player_list)
    local snapshot_list = {}
    for _, player_id in ipairs(_player_list) do
        local player = get_player_snapshot(player_id) 
        if player == nil then
            return 
        end

        snapshot_list[player_id] = player
    end

    return snapshot_list
end
