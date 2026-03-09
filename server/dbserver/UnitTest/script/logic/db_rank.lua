module("db_rank", package.seeall)
--------------------------------------------------------------
-- 全局变量  
--------------------------------------------------------------
g_pack_ar = g_pack_ar or LAr:c_new()
g_unpack_ar = g_unpack_ar or LAr:c_new()

local tinsert, sformat = table.insert, string.format
local c_msgpack, c_msgunpack = cmsgpack.pack, cmsgpack.unpack
local ser_table = utils.serialize_table
local loadstring = loadstring

--排行榜类型
RANK_TYPE_GS        = 1
RANK_TYPE_GOLD      = 2
RANK_TYPE_LEVEL     = 3
RANK_TYPE_TEAM_3V3  = 5
RANK_TYPE_SKILL     = 6
RANK_TYPE_EQUIP     = 7
RANK_TYPE_GOD_EQUIP = 8
RANK_TYPE_TRANSFORM = 9
RANK_TYPE_ARES      = 11
RANK_TYPE_GUILD_PVP = 12
RANK_TYPE_LEIFENG   = 13
RANK_TYPE_GUILD     = 14
RANK_TYPE_LADDER    = 15
RANK_TYPE_HALL_UPVOTE = 16
RANK_TYPE_TOWER     = 17
RANK_TYPE_HONOR_LADDER_DAILY = 18 
RANK_TYPE_GS_JOB1   = 19   -- 野蛮人战力
RANK_TYPE_GS_JOB2   = 20   -- 龙姬战力
RANK_TYPE_GS_JOB3   = 21   -- 斗神战力
RANK_TYPE_GS_JOB4   = 22   -- 萝莉战力
RANK_TYPE_TEAM_CHAMPION = 23
RANK_TYPE_HONOR_TOUR = 24 

RANK_TYPE_ARENA  = 25               -- 排位竞技场


RANK_TYPE_GOLD_MAGNATE_TOTAL      = 26    -- 金币大亨总榜
RANK_TYPE_GOLD_MAGNATE_TODAY      = 27    -- 金币大亨今日

RANK_TYPE_REBORN_MAGNATE_TOTAL      = 28    -- 洗练大亨总榜
RANK_TYPE_REBORN_MAGNATE_TODAY      = 29    -- 洗练大亨今日

RANK_TYPE_STONE_MAGNATE_TOTAL      = 30    -- 宝石大亨总榜
RANK_TYPE_STONE_MAGNATE_TODAY      = 31    -- 宝石大亨今日

RANK_TYPE_MAGIC_MAGNATE_TOTAL      = 32    -- 打磨大亨总榜
RANK_TYPE_MAGIC_MAGNATE_TODAY      = 33    -- 打磨大亨今日

-- 数据库单独字段
RANK_TYPE_ALL_MAGNATE_TOTAL      = 34    -- 总大亨总榜
RANK_TYPE_ALL_MAGNATE_TODAY      = 35    -- 总大亨今日

RANK_MAGNATE_SIZE = 100              -- 资源大亨排行榜最大排名


g_query_rank_key = 
{
    [RANK_TYPE_GS] = {"total_gs", 200, 0},
    [RANK_TYPE_GS_JOB1] = {"total_gs", 200, 1},
    [RANK_TYPE_GS_JOB2] = {"total_gs", 200, 2},
    [RANK_TYPE_GS_JOB3] = {"total_gs", 200, 3},
    [RANK_TYPE_GS_JOB4] = {"total_gs", 200, 4},
    [RANK_TYPE_GOLD] = {"total_gold", 200, 0},
    [RANK_TYPE_LEVEL] = {"level", 200, 0},
    [RANK_TYPE_SKILL] = {"skill_gs", 200, 0},
    [RANK_TYPE_EQUIP] = {"equip_gs", 200, 0},
    [RANK_TYPE_GOD_EQUIP] = {"god_equip_gs", 200, 0},
    [RANK_TYPE_TRANSFORM] = {"all_transform_gs", 200, 0},
    [RANK_TYPE_LEIFENG] = {"donate_value", 10, 0},
}

g_forbid_player_id = 
{
    [284] = true,
    [2057] = true,
    [3640 ] = true,
    [4424] = true
}

g_all_rank_list = g_all_rank_list or {}
g_self_rank_list = g_self_rank_list or {}
--------------------------------------------------------------
-- sql  
--------------------------------------------------------------
GET_TOP_100_SQL = [[
    SELECT player_id, 
           player_name, 
           job_id, 
           model_id, 
           guild_name, 
           total_gs, 
           level, 
           (gold + bind_gold) as total_gold, 
           exp1,
           skill_gs, 
           equip_gs, 
           god_equip_gs,
           all_transform_gs, 
           donate_value
    FROM PLAYER_TBL WHERE not deleted ORDER BY %s DESC 
    LIMIT 0, %d 
]]

GET_TOP_100_SQL_JOB_LIMITED = [[
    SELECT player_id, 
           player_name, 
           job_id, 
           model_id, 
           guild_name, 
           total_gs, 
           level, 
           (gold + bind_gold) as total_gold, 
           exp1,
           skill_gs, 
           equip_gs, 
           god_equip_gs,
           all_transform_gs, 
           donate_value
    FROM PLAYER_TBL WHERE job_id = %d and not deleted ORDER BY %s DESC 
    LIMIT 0, %d 
]]


GET_TOOP_100_SQL_RES_TYPE = "isiisiiliiiiii"

DEFAULT_RANK_DATA = "do local ret = {} return ret end"

SELECT_RANK_SQL = [[
    SELECT
    `rank_type`, 
    `rank_version`, 
    `rank_award_time`, 
    `rank_list`,
    `reward_player_list` 
    from RANK_TBL;
]]

SELECT_RANK_SQL_RESULT_TYPE = "iiias"

REPLACE_RANK_SQL = [[
    REPLACE INTO RANK_TBL ( `rank_type`, `rank_version`, `rank_award_time`, `rank_list`, `reward_player_list` ) VALUES 
    (
        ?,
        ?,
        ?,
        ?,
        ?
    )
]]

REPLACE_RANK_SQL_PARAM_TYPE = "iiias"

INSERT_RANK_LOG_SQL = [[
    INSERT INTO RANK_LOG_TBL ( `rank_type`, `begin_time`, `end_time`, `rank_list` ) VALUES
    (
        ?,
        ?,
        ?,
        ?
    )
]]
INSERT_RANK_LOG_SQL_PARAM_TYPE = "iiis"

SELECT_RANK_LOG_SQL = [[
    SELECT `rank_type`, `begin_time`, `end_time`, unix_timestamp(datetime) as datetime, `rank_list` from RANK_LOG_TBL where rank_type = ? and ? < unix_timestamp(datetime) and unix_timestamp(datetime) < ?;
]]
SELECT_RANK_LOG_SQL_PARAM_TYPE = "iii"
SELECT_RANK_LOG_SQL_RESULT_TYPE = "iiiis"



-- 资源大亨
function init_get_top_resource_magnate_sql(_rank_type, _begin_time, _end_time)

    local magnet_id, is_today = player_t.get_magnate_id(_rank_type)
    local _, _, name = player_t.get_rank_type(magnet_id)
    name = string.lower(name)

    local today_or_total_str = is_today == 1 and "today" or "total"

    local compare_time = _begin_time
    if is_today == 1 then            --今日0点时间
        local now = os.time()
        local year = tonumber(os.date("%Y", now))
        local month = tonumber(os.date("%m", now))
        local day = tonumber(os.date("%d", now))

        compare_time = os.time({year=year, month=month, day=day, hour=0, minute=0, second=0})
    end

    local sql =
    [[
        SELECT a.player_id as player_id, 
               a.player_name as player_name,
               a.head_icon_id as head_icon_id,
               b.%s_magnate_%s,
               b.%s_magnate_%s_rewarded
        FROM PLAYER_TBL a, RESOURCE_MAGNATE_TBL b WHERE a.player_id = b.player_id and b.%s_magnate_%s > 0 and unix_timestamp(a.last_update_time) > %d
        and b.begin_time = %d and b.end_time = %d ORDER BY b.%s_magnate_%s DESC
        LIMIT 0, %d 
    ]]
    return string.format(sql, name, today_or_total_str, name, today_or_total_str, name, today_or_total_str, compare_time,
        _begin_time, _end_time, name, today_or_total_str, RANK_MAGNATE_SIZE)

end

function init_get_top_resource_magnate_fmt(_rank_type)
    local magnet_id, is_today = player_t.get_magnate_id(_rank_type)
    local _, _, name = player_t.get_rank_type(magnet_id)
    name = string.lower(name)

    local today_or_total_str = is_today == 1 and "today" or "total"

    return "player_id", "player_name", "head_icon_id", name..'_magnate_'..today_or_total_str, name..'_magnate_'..today_or_total_str..'_rewarded'

end

-- GET_TOP_RESOURCE_MAGNATE_SQL = 

-- [[
--     SELECT a.player_id as player_id, 
--            a.player_name as player_name, 
--            b.%s,
--            b.
--     FROM PLAYER_TBL a, RESOURCE_MAGNATE_TBL b WHERE a.player_id = b.player_id ORDER BY %s DESC 
--     LIMIT 0, %d 
-- ]]
GET_TOP_RESOURCE_MAGNATE_SQL_RES_TYPE = "isiii"


--------------------------------------------------------------
-- 私有方法  
--------------------------------------------------------------
local function deserialize_rank_info(_ar)
    local rank_type = _ar:read_byte()
    if rank_type == RANK_TYPE_TEAM_3V3 then
        return team_rank_info_t.db_deserialize(rank_type, _ar)
    elseif rank_type == RANK_TYPE_TEAM_CHAMPION then
        return team_champion_rank_info_t.db_deserialize( rank_type, _ar )
    elseif rank_type == RANK_TYPE_GUILD_PVP or rank_type == RANK_TYPE_GUILD then 
        return guild_rank_info_t.db_deserialize(rank_type, _ar)
    elseif rank_type == RANK_TYPE_HALL_UPVOTE or 
        rank_type == RANK_TYPE_TOWER then
        local str = _ar:read_string()
        local func = loadstring(str)
        local rank_info = func()
        return rank_info
    else
        return rank_info_t.db_deserialize(rank_type, _ar) 
    end
end

local function serialize_rank_info(_rank_info, _ar, _rank_type)
    local rank_type = _rank_info.rank_type_ and _rank_info.rank_type_ or _rank_type
    if rank_type == RANK_TYPE_TEAM_3V3 then 
        team_rank_info_t.db_serialize(_rank_info, _ar)
    elseif rank_type == RANK_TYPE_TEAM_CHAMPION then
        team_champion_rank_info_t.db_serialize( _rank_info, _ar )
    elseif rank_type == RANK_TYPE_GUILD_PVP or rank_type == RANK_TYPE_GUILD then 
        guild_rank_info_t.db_serialize(_rank_info, _ar)
    elseif rank_type == RANK_TYPE_HALL_UPVOTE or
        rank_type == RANK_TYPE_TOWER then
        _ar:write_byte( rank_type )
        -- 如果这里用cmsgpack存储，再deserialize_rank_info读出来会被截断,不知道为什么
        local str = ser_table( _rank_info )
        _ar:write_string(str)
    else
        rank_info_t.db_serialize(_rank_info, _ar)
    end
end

local function pack_common_rank_list(_ar, _rank_list, _rank_type)
    local size = #_rank_list 
    _ar:write_int(size) 
    for i=1, size do
        local rank_info = _rank_list[i]
        serialize_rank_info(rank_info, _ar, _rank_type)
    end
end

local function unpack_common_rank_list(_ar)
    local rank_list = {}
    local size = _ar:read_int()
    for i=1, size do
        local rank_info = deserialize_rank_info(_ar)
        rank_list[i] = rank_info
    end
    return rank_list 
end

local function pack_seg_rank_list(_ar, _rank_list, _rank_type)
    local count = 0
    local offset = _ar:get_offset()
    _ar:write_byte(count)

    for seg, list in pairs(_rank_list) do
        _ar:write_byte(seg)
        local size = #list
        _ar:write_ushort(size)
        for i=1, size do
            local rank_info = list[i]
            serialize_rank_info(rank_info, _ar, _rank_type)
        end      
        count = count + 1
    end

    if count > 0 then
       _ar:write_byte_at(count, offset) 
    end
end

local function unpack_seg_rank_list(_ar)
    local rank_list = {}
    local count = _ar:read_byte()
    for i=1, count do
        local seg = _ar:read_byte()
        local size = _ar:read_ushort() 
        local list = {}
        for i=1, size do
            local rank_info = deserialize_rank_info(_ar)
            list[i] = rank_info
        end      
        rank_list[seg] = list
    end
    
    return rank_list
end

local function pack_rank_list(_rank_type, _rank_list)
    local ar = g_pack_ar 
    ar:flush()
    if rank_list_mng.g_common_rank_type[_rank_type] 
        or rank_list_mng.g_manual_update_rank_type[_rank_type] 
        or _rank_type == RANK_TYPE_GUILD 
        or _rank_type == RANK_TYPE_TOWER then
        pack_common_rank_list(ar, _rank_list, _rank_type) 
    elseif rank_list_mng.g_seg_rank_type[_rank_type] then 
        pack_seg_rank_list(ar, _rank_list, _rank_type) 
    end    

    return ar:get_buffer()
end

local function unpack_rank_list(_rank_type, _rank_list_str)
    local ar = g_unpack_ar
    --ar:flush()
    ar:reuse(_rank_list_str)
    local rank_list = {}
    if rank_list_mng.g_common_rank_type[_rank_type] 
        or rank_list_mng.g_manual_update_rank_type[_rank_type] 
        or _rank_type == RANK_TYPE_GUILD 
        or _rank_type == RANK_TYPE_TOWER then
        rank_list = unpack_common_rank_list(ar) 
    elseif rank_list_mng.g_seg_rank_type[_rank_type] then 
        rank_list = unpack_seg_rank_list(ar) 
    end    
    return rank_list
end

--------------------------------------------------------------
-- 外部接口  
--------------------------------------------------------------
function init_self_server_rank_list()
    local player_rank_data = get_db_player_rank_list()

    local split_list = {}
    for rank_type, rank_list in pairs(player_rank_data) do
        rank_list_mng.split_rank_list(split_list, rank_type, rank_list)
    end

    for i=1, #split_list do
        dbserver.remote_call_game("rank_list_mng.split_init_self_server_rank_list", split_list[i])
    end

    dbserver.remote_call_game( "rank_list_mng.init_self_sever_rank_list")
end

function init_all_server_rank_list()
    local rank_data = do_get_rank_data() or {} 
    local split_list = {}
    local data_list = {}
    for rank_type, data in pairs(rank_data) do
        local rank_list = data.rank_list_
        if rank_list_mng.g_common_rank_type[rank_type] 
            or rank_list_mng.g_manual_update_rank_type[rank_type] 
            or rank_type == RANK_TYPE_GUILD 
            or rank_type == RANK_TYPE_TOWER then
            rank_list_mng.split_rank_list(split_list, rank_type, rank_list)
        elseif rank_list_mng.g_seg_rank_type[rank_type] then
            for seg, seg_rank_list in pairs(rank_list) do
                rank_list_mng.split_rank_list(split_list, rank_type, seg_rank_list, seg)
            end
        end

        local data_entry = {
            reward_player_list_ = data.reward_player_list_,
            rank_version_ = data.rank_version_, 
            rank_award_time_ = data.rank_award_time_
        }
        data_list[rank_type] = data_entry
    end

    for i=1, #split_list do
        dbserver.remote_call_game("rank_list_mng.split_init_all_server_rank_list", split_list[i])
    end
    dbserver.remote_call_game( "rank_list_mng.init_all_server_rank_list", data_list)
end

function get_rank_list()
    local player_rank_data = get_db_player_rank_list()
    local split_list = {}
    for rank_type, rank_list in pairs(player_rank_data) do
        rank_list_mng.split_rank_list(split_list, rank_type, rank_list)
    end

    for i=1, #split_list do
        dbserver.remote_call_game("rank_list_mng.on_split_get_rank_list_from_db", split_list[i])
    end

    return dbserver.remote_call_game( "rank_list_mng.on_get_rank_list_from_db")
end

function get_db_player_rank_list()
    local database = dbserver.g_db_character
    local result = {}
    for key, entry in pairs(g_query_rank_key) do
        local job_id = entry[3]
        local sql = nil
        if job_id > 0 then
            sql = sformat(GET_TOP_100_SQL_JOB_LIMITED, job_id, entry[1], entry[2])
        else
            sql = sformat(GET_TOP_100_SQL, entry[1], entry[2])
        end
        if database:c_stmt_format_query( sql, "", GET_TOOP_100_SQL_RES_TYPE ) < 0 then
            c_err( "[db_rank](get_rank_list) c_stmt_format_query on sql:%s failed", sql)
            return result
        end

        local rows = database:c_stmt_row_num()
        local player_infos = {}
        if rows > 0 then
            for i=1, rows do 
                local ret = database:c_stmt_fetch() 
                if ret == 0 then
                    local info = {}
                    info.player_id_ , 
                    info.player_name_, 
                    info.job_,
                    info.model_id_,
                    info.guild_name_, 
                    info.gs_, 
                    info.level_,
                    info.gold_,
                    info.exp_,
                    info.skill_gs_, 
                    info.equip_gs_, 
                    info.god_equip_gs_, 
                    info.all_transform_gs_, 
                    info.donate_value_ 
                    = database:c_stmt_get( "player_id", "player_name","job_id", "model_id", "guild_name", "total_gs", "level", "total_gold", "exp1", "skill_gs", "equip_gs", "god_equip_gs", "all_transform_gs", "donate_value")
                    
                    if not g_forbid_player_id[info.player_id_] then
                        tinsert(player_infos, info)
                    end

                else
                    c_err( "[db_rank](get_rank_list) c_stmt_fetch eror ret:%d on sql:%s failed", ret, sql)
                    return result 
                end
            end
        end

        result[key] = player_infos
    end

    return result
end

-- 资源大亨 不需要1h从数据拉取数据更新
function get_db_player_resource_magnate_rank_list(_begin_time, _end_time)
    local database = dbserver.g_db_character
    local result = {}

    for rank_type = rank_list_mng.RANK_TYPE_RESOURCE_MAGNATE_BAGIN, rank_list_mng.RANK_TYPE_RESOURCE_MAGNATE_END do
        local sql = init_get_top_resource_magnate_sql(rank_type, _begin_time, _end_time)
        if database:c_stmt_format_query( sql, "", GET_TOP_RESOURCE_MAGNATE_SQL_RES_TYPE ) < 0 then
            c_err( "[db_rank](get_rank_list) c_stmt_format_query on sql:%s failed", sql)
            return result
        end

        local rows = database:c_stmt_row_num()
        local player_infos = {}
        if rows > 0 then
            for i=1, rows do
                local ret = database:c_stmt_fetch()
                if ret == 0 then
                    local info = {}
                    local resource_magnate = {}
                    info.resource_magnate_ = resource_magnate
                    info.player_id_ ,
                    info.player_name_,
                    info.head_icon_id_,
                    resource_magnate.value_,
                    resource_magnate.rewarded_
                    = database:c_stmt_get(init_get_top_resource_magnate_fmt(rank_type))

                    -- 玩家身上的奖励领取状态是1 0 2，排行榜只有0 1两个状态[是否领取] 要转换
                    resource_magnate.rewarded_ = (resource_magnate.rewarded_ == player_t.ACTIVITY_STATE_REWARDED) and 1 or 0

                    if not g_forbid_player_id[info.player_id_] then
                        tinsert(player_infos, info)
                    end

                else
                    c_err( "[db_rank](get_rank_list) c_stmt_fetch eror ret:%d on sql:%s failed", ret, sql)
                    return result
                end
            end
        end

        result[rank_type] = player_infos
        c_log("[db_rank](get_db_player_resource_magnate_rank_list) rank_type:%d, player_infos size:%d", rank_type, #player_infos)

    end


    return result
end

function do_get_rank_data( )
    local database = dbserver.g_db_character
    local data = {} 
    if database:c_stmt_format_query( SELECT_RANK_SQL, "", SELECT_RANK_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_rank](do_get_rank_data) c_stmt_format_query on SELECT_RANK_SQL failed")
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return nil 
    elseif rows > 0 then
        for i=1, rows do 
            if database:c_stmt_fetch() == 0 then
                local rank_type = database:c_stmt_get( "rank_type" )
                local rank_version = database:c_stmt_get("rank_version")
                local rank_award_time = database:c_stmt_get("rank_award_time")
                local rank_buf = database:c_stmt_get( "rank_list" )
                local player_list_str = database:c_stmt_get( "reward_player_list" )
                local rank_list = unpack_rank_list(rank_type, rank_buf)
                data[rank_type] =  {
                    rank_version_ = rank_version,
                    rank_award_time_ = rank_award_time, 
                    rank_list_ = rank_list, 
                    reward_player_list_ = player_list_str, 
                }
            else
                return nil
            end
        end
    end
    return data 
end

function save_rank_split(_split)
    if not _split then
        return 
    end
    
    local rank_type = _split.rank_type_
    local list = _split.list_
    local seg = _split.seg_ or 0
    local rank_list = {}  
    if rank_list_mng.g_common_rank_type[rank_type] 
        or rank_list_mng.g_manual_update_rank_type[rank_type]
        or rank_type == RANK_TYPE_GUILD 
        or rank_type == RANK_TYPE_TOWER then
        rank_list = g_all_rank_list[rank_type] or {}
        g_all_rank_list[rank_type] = rank_list
    elseif rank_list_mng.g_seg_rank_type[rank_type] then
        local seg_rank_list = g_all_rank_list[rank_type] or {}
        rank_list = seg_rank_list[seg] or {}
        seg_rank_list[seg] = rank_list
        g_all_rank_list[rank_type] = seg_rank_list
    end 

    local count = 0
    for rank, info in pairs(list) do
        rank_list[rank] = info
        count = count + 1
    end
    c_log("[db_rank](save_split_rank) rank_type:%d seg:%d count:%d", rank_type, seg, count)
end

function do_save_rank_data(_data)
    local rank_type = _data.rank_type_
    local rank_version = _data.rank_version_ 
    local rank_award_time = _data.rank_award_time_
    local reward_player_list = _data.reward_player_list_
    local rank_list =  g_all_rank_list[rank_type] or {}
    local rank_buf = ""
    if rank_list then
        rank_buf = pack_rank_list(rank_type, rank_list)
    end
    local ret =  dbserver.g_db_character:c_stmt_format_modify(
        REPLACE_RANK_SQL,
        REPLACE_RANK_SQL_PARAM_TYPE, 
        rank_type, 
        rank_version,
        rank_award_time, 
        rank_buf,
        reward_player_list)
    if not ret then
        return 
    end 
                
    g_all_rank_list[rank_type] = nil 
    c_log("[db_rank](do_save_rank_data) rank_type:%d rank_version:%d", rank_type, rank_version)
end

function init_resource_magnate_rank_list(_begin_time, _end_time)

    local player_resource_magnate_data = get_db_player_resource_magnate_rank_list(_begin_time, _end_time)

    local split_list = {}
    for rank_type, rank_list in pairs(player_resource_magnate_data) do
        rank_list_mng.split_rank_list(split_list, rank_type, rank_list)
    end

    for i=1, #split_list do
        dbserver.remote_call_game("rank_list_mng.split_init_self_server_rank_list", split_list[i])
    end

    dbserver.remote_call_game( "rank_list_mng.init_self_sever_rank_list")

end

-- 资源大亨 排行榜日志
function save_resource_magnate_log(_rank_type, _begin_time, _end_time, _rank_list_str)
    local ret =  dbserver.g_db_character:c_stmt_format_modify( INSERT_RANK_LOG_SQL, INSERT_RANK_LOG_SQL_PARAM_TYPE, _rank_type, _begin_time, _end_time, _rank_list_str )
    if ret < 0 then
        c_err("db_rank.save_resource_magnate_log err")
    else
        c_trace("db_rank.save_resource_magnate_log, ret:%d", ret)
    end
    return ret
end

function do_get_rank_log(_rank_type, _begin_time, _end_time)
    local database = dbserver.g_db_character
    local data = {}
    if database:c_stmt_format_query(SELECT_RANK_LOG_SQL, SELECT_RANK_LOG_SQL_PARAM_TYPE, _rank_type, _begin_time, _end_time, SELECT_RANK_LOG_SQL_RESULT_TYPE) < 0 then
        c_err( "[db_rank](do_get_rank_log) c_stmt_format_query on SELECT_RANK_LOG_SQL failed")
        return -1, nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return 0, data
    elseif rows > 0 then
        if rows > 1 then
            return -3, nil
        else
            if database:c_stmt_fetch() == 0 then
                local rank = {}
                rank.rank_type_ = database:c_stmt_get("rank_type")
                rank.begin_time_ = database:c_stmt_get("begin_time")
                rank.end_time_ = database:c_stmt_get("end_time")
                rank.save_time_ = database:c_stmt_get("datetime")
                rank.rank_list_ = loadstring(database:c_stmt_get("rank_list"))()
                table.insert(data, rank)
            else
                return  -2, nil
            end
        end
    end
    return 0, data
end

function get_rank_log(_rank_type, _begin_time, _end_time, _callback, _param)
    local error_code, rank_log_list = do_get_rank_log(_rank_type, _begin_time, _end_time)
    dbserver.remote_call_game(_callback, error_code, rank_log_list or {}, _param)
end
