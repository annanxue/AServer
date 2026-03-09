module( "db_arena", package.seeall )

local sformat, tinsert, tremove, mrandom = string.format, table.insert, table.remove, math.random


SELECT_ALL_ARENA_PLAYER_SQL = [[
    SELECT
    `id`,
    `rank`,
    `is_robot`,
    `team`,
    `battled_times`,
    `buy_times`,
    `battle_report`,
    `name`,
    `gs`,
    `head_icon_id`,
    `job_id`,
    `guild_name`,
    `rank_top`
    from ARENA_TBL
    order by rank;
]]

SELECT_ALL_ARENA_PLAYER_SQL_PARAM_TYPE = ""
SELECT_ALL_ARENA_PLAYER_SQL_RESULT_TYPE = "fiisiissiiisi"


SELECT_ARENA_PLAYER_BY_ID_SQL = [[
    SELECT
    `id`,
    `rank`,
    `is_robot`,
    `team`,
    `battled_times`,
    `buy_times`,
    `battle_report`,
    `name`,
    `gs`,
    `head_icon_id`,
    `job_id`,
    `guild_name`,
    `rank_top`
    from ARENA_TBL where id = ?;
]]
SELECT_ARENA_PLAYER_BY_ID_SQL_RESULT_TYPE = "fiisiissiiisi"

REPLACE_ARENA_PLAYER_SQL = [[
    REPLACE INTO ARENA_TBL 
    ( `id`, `rank`, `is_robot`, `team`, `battled_times`, `buy_times`,
        `battle_report`, `name`, `gs`, `head_icon_id`, `job_id`, `guild_name`, `rank_top` ) VALUES
    (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?
    );
]]
REPLACE_ARENA_PLAYER_SQL_PARAM_TYPE = "iiisiissiiisi"


--SELECT_TOWER_RANK_LIST_SQL = [[
--    SELECT
--        PLAYER_TBL.`player_id`,
--        PLAYER_TBL.`player_name`,
--        TOWER_TBL.`max_pass_id`,
--        TOWER_TBL.`min_cost_time`,
--        TOWER_TBL.`pass_timestamp`,
--        PLAYER_TBL.`job_id`,
--        PLAYER_TBL.`model_id`,
--        PLAYER_TBL.`max_gs`
--    FROM TOWER_TBL LEFT JOIN PLAYER_TBL 
--        ON TOWER_TBL.player_id = PLAYER_TBL.player_id
--    WHERE max_pass_id != 0
--        ORDER BY max_pass_id DESC, min_cost_time ASC, pass_timestamp ASC
--    LIMIT ]] .. share_const.TOWER_RANK_NUM .. ";"

--SELECT_TOWER_RANK_LIST_SQL_RESULT_TYPE = "isiiiiii"

--DELETE_TOWER_RANK_LIST_SQL = [[
--    UPDATE TOWER_TBL set max_pass_id = 0, min_cost_time = 0, pass_timestamp = 0;
--]]

--REPLACE_TOWER_SQL = [[
--    REPLACE INTO TOWER_TBL 
--    ( `player_id`, `data`, `max_pass_id`, `min_cost_time`, `pass_timestamp` ) VALUES
--    (
--        ?,
--        ?,
--        ?,
--        ?,
--        ?
--    )
--]]
--REPLACE_TOWER_SQL_PARAM_TYPE = "isiii"

function do_get_all_arena_player()
    local all_arena_player = get_all_arena_player()
    c_assert( all_arena_player, "[db_arena](do_get_all_arena_player) failed to get arena player data! please check")

    if not all_arena_player or utils.table.size(all_arena_player) == 0 then
        dbserver.remote_call_game( "arena_mng.on_get_all_arena_player", {} )
    else
--        for _, arena_player in ipairs( all_arena_player ) do
--            dbserver.remote_call_game( "arena_mng.on_get_all_arena_player", arena_player )
--        end
        
--        dbserver.remote_call_game( "arena_mng.on_get_all_arena_player", all_arena_player )

--        get_all_robot_timer = g_timermng:c_add_timer( 1, timers.ARENA_INIT_SAVE_ROBOT_TIMER, 0, 0, 1 )

        local arena_player_list = {}
        for i = 1, #all_arena_player do
            table.insert(arena_player_list, all_arena_player[i])

            if i % 100 == 0 or i == #all_arena_player then
--                dbserver.remote_call_game( "arena_mng.on_get_all_arena_player", arena_player_list )
                dbserver.split_remote_call_game( "arena_mng.on_get_all_arena_player", arena_player_list )
                arena_player_list = {}
            end
        end

    end

end


function do_save_arena_player(_arena_player_str)

    local arena_player = loadstring(_arena_player_str )()

    if not arena_player.name_ or string.len(arena_player.name_) == 0 then 
        -- 名字为空  ， 肯定是机器人， 所以要随机一个名字。机器人性别暂时随机。 
        local robot_name = db_random_names.get_robot_random_name( mrandom(1 , 2) )
        arena_player.name_ = robot_name 
        dbserver.remote_call_game( "arena_mng.update_arena_player_name", arena_player.id_ , robot_name )
    end 

    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_ARENA_PLAYER_SQL, REPLACE_ARENA_PLAYER_SQL_PARAM_TYPE,
            arena_player.id_, arena_player.rank_, arena_player.is_robot_ and 1 or 0, utils.serialize_table(arena_player.team_),
            arena_player.battled_times_, arena_player.buy_times_, utils.serialize_table(arena_player.battle_report_),
            arena_player.name_, arena_player.gs_, arena_player.head_icon_id_, arena_player.job_id_, arena_player.guild_name_, arena_player.rank_top_)

end

function do_save_arena_player_list(_arena_player_list_str)

    local arena_player_list = loadstring(_arena_player_list_str )() 

    for k,v in pairs(arena_player_list) do
        local arena_player = v
        dbserver.g_db_character:c_stmt_format_modify( REPLACE_ARENA_PLAYER_SQL, REPLACE_ARENA_PLAYER_SQL_PARAM_TYPE,
            arena_player.id_, arena_player.rank_, arena_player.is_robot_ and 1 or 0, utils.serialize_table(arena_player.team_),
            arena_player.battled_times_, arena_player.buy_times_, utils.serialize_table(arena_player.battle_report_),
            arena_player.name_, arena_player.gs_, arena_player.head_icon_id_, arena_player.job_id_, arena_player.guild_name_, arena_player.rank_top_)
    end

end


function get_all_arena_player()
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_ARENA_PLAYER_SQL, SELECT_ALL_ARENA_PLAYER_SQL_PARAM_TYPE, SELECT_ALL_ARENA_PLAYER_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_arena](get_all_arena_player)  SELECT_ALL_ARENA_PLAYER_SQL failed" )
        return nil
    end

    local all_arena_player = {}
    local rows = database:c_stmt_row_num()

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local id, rank, is_robot, team, battled_times, buy_times, battle_report,
                    name, gs, head_icon_id, job_id, guild_name, rank_top
                = database:c_stmt_get( "id", "rank", "is_robot", "team", "battled_times",
                    "buy_times", "battle_report", "name", "gs", "head_icon_id", "job_id", "guild_name", "rank_top")

            -- 暂时这样做，防止机器人名字为空
            if not name or string.len(name) == 0 then
                name = db_random_names.get_robot_random_name( mrandom(1 , 2) )
            end

            tinsert(all_arena_player, { id_ = id, rank_ = rank, is_robot_ = is_robot == 1 and true or false, team_ = loadstring( team )(),
                battled_times_ = battled_times, buy_times_ = buy_times, battle_report_ = loadstring(battle_report )(),
                name_ = name, gs_ = gs, head_icon_id_ = head_icon_id, job_id_ = job_id, guild_name_ = guild_name, rank_top_ = rank_top})
        else
            c_err( "[db_guild_system](get_all_arena_player) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end

    time = c_cpu_ms() - time
    c_prof( "[db_guild_system](get_all_arena_player)rows:%d, use time: %dms", rows, time )

    return all_arena_player
end
