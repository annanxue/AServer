module( "timers", package.seeall )

local sformat, min = string.format, math.min

g_func_map = g_func_map or {}

---------------------------------------------
--              timer id 
---------------------------------------------

LOGIN_CLIENT_HEARTBEAT = 1
CLOSE_OLD_CONNECTION = 2
REJOIN_TIMEOUT = 3
LOGIN_QUEUE_RANK_TIMER = 4
CLOSE_SUSPEND_SESSION_TIMER = 5
PRINT_ONLINE_COUNT_TIMER = 6
SEND_SHOP_TO_CLIENT = 7
SYSTEM_BULLETIN_TIMER = 8
EVENT_CARNIVAL_FORECAST_TIMER = 9
REFRESH_RUNNER_ITEMS = 10
CHECK_GAMESVR_CFG = 11
SAVE_GUILD_TIMER = 12
REFRESH_GUILD_NEXT_FRAME_TIMER = 13
REFRESH_EACH_GUILD = 14
EVENT_ARES_GAMESVR_TIMER = 15
GUILD_DAILY_PVP_ENROLL_BULLETIN_TIMER = 16
GUILD_DAILY_PVP_PREPARE_BULLETIN_TIMER = 17
GUILD_DAILY_PVP_SEND_INIT_DATA_TIMER = 18
EVENT_GOD_DIVINATION_START = 19
EVENT_GOD_DIVINATION_STOP = 20
EVENT_TAMRIEL_JOIN_BATTLE_TIMER = 21
TEAM_LADDER_GAME_INIT_TIMER = 22
TEAM_LADDER_LOCK_PLAYER_TIMER = 23
TEAM_LADDER_WAIT_TEAM_ADD_MEMBER_TIMER = 24
TEAM_LADDER_MATCH_STATE_WAIT_ACK_TIMEOUT = 25
TEAM_LADDER_SAVE_LADDER_TEAM_TIMER = 26
TEAM_LADDER_LOCK_TEAM_TIMEOUT = 27
BATTLE_JUSTICE_RECOVER_TIMEOUT = 28
TEAM_CHAMPION_GAME_INIT_TIMER = 29
TEAM_CHAMPION_BROADCAST_RESULT_TIMER = 30
TEAM_LADDER_BROADCAST_WIN_MSG_TIMER = 31
ARES_WITNESS_LIKE_RESULT_TIMER = 32
TEAM_CHAMPION_CHECK_LIKE_RESULT_TIMER = 33
EVENT_CARNIVAL_AWARD_TIMER = 34
SAVE_ONE_GOLD_LUCKY_DRAW_TIMER = 35
ARENA_PER_DAY_REWARD_TIMER = 36
ARENA_INIT_SAVE_ROBOT_TIMER = 37
GM_ACTIVITY_CHECK_TIMER                 = 38            -- gm活动检查过期定时器
GM_ACTIVITY_AVATAR_LOTTERY              = 39            -- gm活动变身抽奖

---------------------------------------------
--              timer func
---------------------------------------------
g_func_map[LOGIN_CLIENT_HEARTBEAT] = function( _p1, _p2 )
    return loginclient.on_heartbeat_timeout(_p1, _p2) 
end

g_func_map[CLOSE_OLD_CONNECTION] = function( _p1, _p2 )
    login_mng.close_old_connection_timer_callback( _p1 )
    return 0
end

g_func_map[REJOIN_TIMEOUT] = function( _player_id )
    local bf_player = bfclient.remove_bf_player( _player_id )
    if bf_player then
        for player_data_op_id, player_data_op in pairs( bf_player.player_data_ops_ ) do
            local func_for_offline_player = player_data_op.func_for_offline_player_
            if func_for_offline_player then
                func_for_offline_player( _player_id, player_data_op.params_ )
            end
        end
    end
    c_log( "[timers](g_func_map[REJOIN_TIMEOUT]) player_id: %d", _player_id )
    return 0
end

g_func_map[LOGIN_QUEUE_RANK_TIMER] = function( _p1, _p2 )
   login_mng.refresh_queue_rank_timer_callback()
   return 1
end

g_func_map[CLOSE_SUSPEND_SESSION_TIMER] = function( _p1, _p2 )
    login_mng.close_session_timer_callback( _p1 )
    return 0
end

g_func_map[PRINT_ONLINE_COUNT_TIMER] = function( _p1, _p2 )
    login_mng.print_online_count_timer_callback()
    return 1
end

g_func_map[SEND_SHOP_TO_CLIENT] = function( _player_id )
    shop_mng.timer_send_shop_to_client( _player_id )
    return 0
end

g_func_map[SYSTEM_BULLETIN_TIMER] = function( _p1, _p2 )
    return bulletin_mng.timer_send_bulletion( _p1, _p2 )
end

g_func_map[EVENT_CARNIVAL_FORECAST_TIMER] = function( _p1, _p2 )
    return time_event_carnival.forecast_timer_callback()
end

g_func_map[REFRESH_RUNNER_ITEMS] = function( _p1, _p2 )
    runner_mng.refresh_items( _p1, _p2 )
    return 1
end

g_func_map[CHECK_GAMESVR_CFG] = function( _p1, _p2 )
    game_zone_mng.check_gamesvr_cfg( _p1, _p2 )
    return 1
end

g_func_map[SAVE_GUILD_TIMER] = function( _p1, _p2 )
    return guild_mng.on_save_guild_timeout( _p1, _p2 )
end

g_func_map[REFRESH_GUILD_NEXT_FRAME_TIMER] = function( _p1, _p2 )
    guild_mng.refresh_all_guild()
    return 0
end

g_func_map[REFRESH_EACH_GUILD] = function( _p1, _p2 )
    guild_mng.refresh_guild( _p1 )
    return 0
end

g_func_map[EVENT_ARES_GAMESVR_TIMER] = function( _p1, _p2 )
    return event_ares_game.ares_gamesvr_timer_callback( _p1, _p2 )
end

g_func_map[GUILD_DAILY_PVP_ENROLL_BULLETIN_TIMER] = function()
    return guild_daily_pvp.on_enroll_bulletin_timer()
end

g_func_map[GUILD_DAILY_PVP_PREPARE_BULLETIN_TIMER] = function()
    guild_daily_pvp.on_prepare_bulletin_timer()
    return 0
end

g_func_map[GUILD_DAILY_PVP_SEND_INIT_DATA_TIMER] = function()
    return guild_daily_pvp.on_send_init_data_timer()
end

g_func_map[EVENT_GOD_DIVINATION_START] = function( _p1, _p2 )
    event_god_divination.start_event()
    return 0
end

g_func_map[EVENT_GOD_DIVINATION_STOP] = function( _p1, _p2 )
    event_god_divination.stop_event()
    return 0
end

g_func_map[EVENT_TAMRIEL_JOIN_BATTLE_TIMER] = function( _p1, _p2 )
    tamriel_relic.join_battle_timeout_call_back( _p1, _p2 )
    return 0
end

g_func_map[TEAM_LADDER_GAME_INIT_TIMER] = function()
    return event_team_ladder.on_game_init_timer()
end

g_func_map[TEAM_LADDER_LOCK_PLAYER_TIMER] = function( _team_id )
    event_team_ladder.on_try_lock_player_timeout( _team_id )
    return 0
end

g_func_map[TEAM_LADDER_WAIT_TEAM_ADD_MEMBER_TIMER] = function( _player_id )
    event_team_ladder.on_wait_team_add_member_timeout( _player_id )
    return 0
end

g_func_map[TEAM_LADDER_MATCH_STATE_WAIT_ACK_TIMEOUT] = function( _team_id, _battle_state )
    event_team_ladder.on_match_state_wait_ack_timeout( _team_id, _battle_state )
    return 0
end

g_func_map[TEAM_LADDER_SAVE_LADDER_TEAM_TIMER] = function( _team_id )
    return event_team_ladder.on_save_team_timer( _team_id )
end

g_func_map[TEAM_LADDER_LOCK_TEAM_TIMEOUT] = function( _team_id )
    event_team_ladder.on_lock_team_timeout( _team_id )
    return 0
end

g_func_map[ BATTLE_JUSTICE_RECOVER_TIMEOUT ] = function( _player_id, _, _timer_index ) 
    player_t.on_battle_justice_punish_timeout( _player_id, _timer_index )  
end

g_func_map[TEAM_CHAMPION_GAME_INIT_TIMER] = function()
    return event_team_champion.on_game_init_timer()
end

g_func_map[TEAM_CHAMPION_BROADCAST_RESULT_TIMER] = function()
    event_team_champion.on_broadcast_result_timer()
    return 0
end

g_func_map[TEAM_LADDER_BROADCAST_WIN_MSG_TIMER] = function()
    return event_team_ladder.on_broadcast_win_msg_timer()
end

g_func_map[ARES_WITNESS_LIKE_RESULT_TIMER] = function( _p1, _p2 )
    ares_witness_game.reset_like_result_timer_callback()
    return 0
end

g_func_map[TEAM_CHAMPION_CHECK_LIKE_RESULT_TIMER] = function()
    return event_team_champion.on_check_like_result_timer()
end

g_func_map[EVENT_CARNIVAL_AWARD_TIMER] = function( _p1, _p2 )
    return player_t.carnival_award_timer_callback( _p1, _p2 )
end

g_func_map[SAVE_ONE_GOLD_LUCKY_DRAW_TIMER] = function( _p1, _p2 ) 
    return event_one_gold_lucky_draw.save_data()
end

g_func_map[ARENA_PER_DAY_REWARD_TIMER] = function( _p1, _p2 ) 
    return arena_mng.do_day_arena_reward_formula()
end

g_func_map[ARENA_INIT_SAVE_ROBOT_TIMER] = function( _p1, _p2 ) 
    return arena_mng.do_init_save_robot()
end

g_func_map[GM_ACTIVITY_CHECK_TIMER] = function(p1, _p2)
    return activity_mng:gm_activity_check_timer_callback()
end

g_func_map[GM_ACTIVITY_AVATAR_LOTTERY] = function(_begin_time, _end_time)
    return activity_mng:gm_activity_avatar_lottery_callback(_begin_time, _end_time)
end