module( "timers", package.seeall )

local sformat = string.format

g_func_map = g_func_map or {}

-- timer id:
GAME_HEARTBEAT = 1
PLAYER_MIGRATE_TIMEOUT = 2
CLOSE_CLIENT_CONNECTION = 3
TAMRIEL_MATCH_TIMEOUT = 4
PLAYER_JOIN_HONOR_HALL_TIMEOUT = 7
CHECK_GAMESVR_CFG = 9
RANK_ON_START_SERVER = 10
RANK_AWARD_TIMER = 11
COMMON_RANK_TIMER = 12
COMMON_RANK_WAIT_TIMER = 13
MILLIONAIRE_PVP_GOTO_NEXT_ROUND = 14
TOURNAMENT_ROUND_TIMER = 16
EVENT_ARES_COMMON_TIMER = 17
TOURNAMENT_ROUND_TIMEOUT_TIMER = 20
GUILD_MERCHANT_RUNNER_REFRESH_ITEMS = 21
GUILD_DAILY_PVP_PREPARE_TIME_UP = 22
GUILD_DAILY_PVP_ENROLL_TIME_UP = 23
GUILD_DAILY_PVP_MATCH_GUILD_TIMER = 24
GUILD_DAILY_PVP_SEND_BATTLE_REQUEST_TIMER = 25
GUILD_DAILY_PVP_WAIT_RESULT_TIME_UP = 26
GUILD_INST_PLAYER_REVIVE = 27
TRADE_ITEM_SOLD_OUT_TIMER = 28
TRADE_INIT_TIMER = 29
LOCK_CHECK_TIMER = 30
GUILD_DAILY_PVP_BATTLE_TIME_UP = 31
GUILD_DAILY_PVP_FIGHTER_TIMER = 32
GUILD_DAILY_PVP_DUEL_TIME_UP = 33
GUILD_DAILY_PVP_CLEAN_DUEL_TIMER = 34
GUILD_DAILY_PVP_KICK_PLAYER_TIMER = 35
GUILD_DAILY_PVP_RANK_ZONE_TIMER = 36
AUCTION_ITEM_PUBLIC_TIMER = 37
AUCTION_ITEM_BID_TIMER = 38
CHECK_AUCTION_TIMER = 39
TRADE_STATISTIC_RESET_TIMER = 40
TRADE_STATISTIC_INIT_TIMER = 41
PREVENT_KILL_CHEAT = 45
GUILD_DAILY_PVP_SYNC_FIGHTER_INFO_TIMER = 46
FIVE_PVP_DUEL_OPEN_DOOR_TIMER = 47
GUILD_DAILY_PVP_DUEL_OPEN_DOOR_TIMER = 48
EVENT_LADDER_COMMON_TIMER = 49
TEAM_LADDER_ENLARGE_RANGE_TIMER = 50
TEAM_LADDER_WAIT_TEAM_CONFIRM_TIMER = 51
TEAM_LADDER_REMOVE_ENEMY_TIMER = 52
TEAM_LADDER_REMOVE_PERISH_TIMER = 53
TEAM_LADDER_SELECT_MODEL_TIMEUP = 54
QUICK_MATCH_CONFIRM_TIMER = 55
QUICK_MATCH_WAIT_TIMER = 56
TEAM_LADDER_CHECK_MATCH_TIMEOUT_TIMER = 57
TEAM_LADDER_REFRESH_GRADE_TIMER = 58
HONOR_LADDER_WEEKLY_MATCH_DEMO_TIMER = 59
HONOR_TOURNAMENT_ROUND_TIMEOUT_TIMER = 60
HONOR_TOURNAMENT_ROUND_TIMER = 61
TEAM_CHAMPION_FORECAST_ENROLL_END_TIMER = 62
TEAM_CHAMPION_ENROLL_TIME_UP = 63
TEAM_CHAMPION_REST_TIME_UP = 64
TEAM_CHAMPION_SHOW_RESULT_TIME_UP = 65 
TEAM_CHAMPION_WAIT_PLAYER_JOIN_BATTLE_TIME_UP = 66 
TEAM_CHAMPION_ONE_GAME_BATTLE_TIME_UP = 67
TEAM_CHAMPION_READY_GO_TIME_UP = 68
TEAM_CHAMPION_KICK_PLAYER_TIMER = 69
TEAM_CHAMPION_SELECT_MODEL_TIMEUP = 70
TEAM_CHAMPION_WAIT_RESULT_TIME_UP = 71
HONOR_TOURN_BROADCAST_CHAMPION_TIMER = 72
HONOR_LADDER_BROADCAST_FIVE_WIN = 73

g_func_map[GAME_HEARTBEAT] = function( p1, p2, index )    
    bfsvr.check_heart_beat( p1 )
    return 1
end

g_func_map[PLAYER_MIGRATE_TIMEOUT] = function( _svr_id, _player_id )
    bfsvr.on_player_migrate_timeout( _svr_id, _player_id )
    return 0
end

g_func_map[CLOSE_CLIENT_CONNECTION] = function( _p1, _p2 )
    bfsvr.close_connection_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[TAMRIEL_MATCH_TIMEOUT] = function( _p1, _p2 )
    tamriel_relic_center.on_match_timeout_callback( _p1, _p2 )
    return 0
end

g_func_map[PLAYER_JOIN_HONOR_HALL_TIMEOUT] = function( _p1, _p2 )
    honor_hall.join_time_out_check(_p1, _p2 )
    return 0
end

g_func_map[COMMON_RANK_TIMER] = function (_p1, _p2)
    rank_list_mng.begin_all_server_common_rank()
    return 1
end

g_func_map[COMMON_RANK_WAIT_TIMER] = function (_zone_id, _p2)
    rank_list_mng.complete_common_rank(_zone_id)
    return 0
end

g_func_map[CHECK_GAMESVR_CFG] = function( _p1, _p2 )
    bf_zone_mng.check_gamesvr_cfg()
    return 1
end

g_func_map[RANK_ON_START_SERVER] = function( _p1, _p2)
    rank_list_mng.rank_on_start_server()
end

g_func_map[RANK_AWARD_TIMER] = function( _p1, _p2)
    rank_list_mng.begin_all_server_rank_award()
    return 1
end

g_func_map[MILLIONAIRE_PVP_GOTO_NEXT_ROUND] = function( _p1, _p2 )
    millionaire_pvp.process_event_and_go_to_next_round( _p1, _p2 )
    return 1
end

g_func_map[TOURNAMENT_ROUND_TIMER] = function( _p1, _p2 )
    event_ares_center.tournament_round_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[HONOR_TOURNAMENT_ROUND_TIMER] = function( _p1, _p2 )
    honor_tournament_center.honor_tournament_round_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[EVENT_ARES_COMMON_TIMER] = function( _p1, _p2 )
    return event_ares_bf.common_timer_callback( _p1, _p2 )
end

g_func_map[EVENT_LADDER_COMMON_TIMER] = function( _p1, _p2 )
    return honor_ladder_bf.ladder_common_timer_callback( _p1, _p2 )
end

g_func_map[HONOR_LADDER_WEEKLY_MATCH_DEMO_TIMER] = function( _p1, _p2 )
    honor_ladder_weekly.honor_ladder_weekly_demo_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[TOURNAMENT_ROUND_TIMEOUT_TIMER] = function( _p1, _p2 )
    event_ares_center.tournament_round_timeout_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[HONOR_TOURNAMENT_ROUND_TIMEOUT_TIMER] = function( _p1, _p2 )
    honor_tournament_center.honor_tournament_round_timeout_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[HONOR_TOURN_BROADCAST_CHAMPION_TIMER] = function( _p1, _p2 )
    honor_tournament_center.honor_tour_on_timer_to_broadcast_champion( _p1, _p2 )
    return 0
end

g_func_map[HONOR_LADDER_BROADCAST_FIVE_WIN] = function( _p1, _p2 )
    honor_ladder_bf.on_timer_to_send_ladder_world__msg( _p1, _p2 )
    return 0
end

g_func_map[GUILD_MERCHANT_RUNNER_REFRESH_ITEMS] = function( _p1, _p2 )
    guild_mng_prof_task.refresh_items( _p1, _p2 )
    return 1
end

g_func_map[GUILD_DAILY_PVP_ENROLL_TIME_UP] = function()
    guild_daily_pvp.on_enroll_time_up() 
    return 0
end

g_func_map[GUILD_DAILY_PVP_PREPARE_TIME_UP] = function()
    guild_daily_pvp.on_prepare_time_up() 
end

g_func_map[GUILD_DAILY_PVP_MATCH_GUILD_TIMER] = function()
    return guild_daily_pvp.on_match_guild_timer() 
end

g_func_map[GUILD_DAILY_PVP_SEND_BATTLE_REQUEST_TIMER] = function()
    return guild_daily_pvp.on_send_battle_request_timer() 
end

g_func_map[GUILD_DAILY_PVP_WAIT_RESULT_TIME_UP] = function()
    guild_daily_pvp.on_wait_battle_result_time_up() 
    return 0
end

g_func_map[GUILD_INST_PLAYER_REVIVE] = function( _p1, _p2 )
     wild_mng.on_player_revive( _p1, _p2 )
     return 0
end

g_func_map[TRADE_ITEM_SOLD_OUT_TIMER] = function( _p1, _p2 )
    trade_mng.check_trade_item_sold_out()
    return 1
end

g_func_map[TRADE_INIT_TIMER] = function( _p1, _p2 )
    trade_mng.init_trade()
    return 0 
end

g_func_map[LOCK_CHECK_TIMER] = function( _p1, _p2 )
    lock_mng.check_lock_timeout()
    return 1 
end

g_func_map[GUILD_DAILY_PVP_BATTLE_TIME_UP] = function( _inst_index )
    guild_daily_pvp.on_battle_time_up( _inst_index ) 
    return 0
end

g_func_map[GUILD_DAILY_PVP_FIGHTER_TIMER] = function( _inst_index, _timer_sn )
    guild_daily_pvp.on_fighter_timer( _inst_index, _timer_sn ) 
    return 0
end

g_func_map[GUILD_DAILY_PVP_DUEL_TIME_UP] = function( _inst_index, _duel_index )
    guild_daily_pvp.on_duel_time_up( _inst_index, _duel_index ) 
    return 0
end

g_func_map[GUILD_DAILY_PVP_CLEAN_DUEL_TIMER] = function( _inst_index, _duel_index )
    guild_daily_pvp.clean_duel( _inst_index, _duel_index ) 
    return 0
end

g_func_map[GUILD_DAILY_PVP_KICK_PLAYER_TIMER] = function( _inst_index )
    guild_daily_pvp.on_kick_all_player_timer( _inst_index ) 
    return 0
end

g_func_map[GUILD_DAILY_PVP_RANK_ZONE_TIMER] = function()
    return guild_daily_pvp.on_rank_zone_timer() 
end

g_func_map[AUCTION_ITEM_PUBLIC_TIMER] = function( _server_id, _db_item_id )
    auction_mng.on_item_public_timeout( _server_id, _db_item_id )
    return 0
end

g_func_map[AUCTION_ITEM_BID_TIMER] = function( _server_id, _db_item_id )
    auction_mng.on_item_bid_timeout( _server_id, _db_item_id )
    return 0
end

g_func_map[CHECK_AUCTION_TIMER] = function( _p1, _p2 )
    auction_mng.check_auction_timeout()
    return 0 
end

g_func_map[TRADE_STATISTIC_RESET_TIMER] = function(_p1, _p2)
    trade_statistic_mng.on_reset_statistic()
    return 1
end

g_func_map[TRADE_STATISTIC_INIT_TIMER] = function(_p1, _p2)
    trade_statistic_mng.init_trade_statistic()
    return 1
end

g_func_map[PREVENT_KILL_CHEAT] = function(_p1, _p2)
    if g_bfsvr then
        local player = ctrl_mng.get_ctrl( _p1 )
        if not player or not player:is_player() then
            return 0
        end
        player:on_time_out_check_kill_record( _p2 )
    end
    return 0
end

g_func_map[GUILD_DAILY_PVP_SYNC_FIGHTER_INFO_TIMER] = function( _inst_index )
    return guild_daily_pvp.on_sync_fighter_info_timer( _inst_index ) 
end

g_func_map[FIVE_PVP_DUEL_OPEN_DOOR_TIMER] = function( _p1, _p2 )
    five_pvp.duel_open_door_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[GUILD_DAILY_PVP_DUEL_OPEN_DOOR_TIMER] = function( _p1, _p2 )
    guild_daily_pvp.duel_open_door_timer_callback( _p1, _p2 )
    return 0
end

g_func_map[TEAM_LADDER_ENLARGE_RANGE_TIMER] = function()
    return event_team_ladder.on_enlarge_range_timer()
end

g_func_map[TEAM_LADDER_WAIT_TEAM_CONFIRM_TIMER] = function( _sn )
    event_team_ladder.on_wait_team_confirm_timeout( _sn )
    return 0
end

g_func_map[TEAM_LADDER_REMOVE_ENEMY_TIMER] = function()
    return event_team_ladder.on_remove_enemy_timer()
end

g_func_map[TEAM_LADDER_REMOVE_PERISH_TIMER] = function()
    return event_team_ladder.on_remove_perish_timer()
end

g_func_map[TEAM_LADDER_SELECT_MODEL_TIMEUP] = function( _inst_index )
    event_team_ladder.on_select_model_timeup( _inst_index )
    return 0
end

g_func_map[QUICK_MATCH_CONFIRM_TIMER] = function( _match_id )
    team_inst.on_quick_match_confirm_timeout( _match_id )
    return 0
end

g_func_map[QUICK_MATCH_WAIT_TIMER] = function( _player_id, _server_id )
    team_inst.on_wait_quick_match_timeout( _player_id, _server_id )
    return 0
end

g_func_map[TEAM_LADDER_CHECK_MATCH_TIMEOUT_TIMER] = function()
    return event_team_ladder.on_check_match_timeout_timer()
end

g_func_map[TEAM_LADDER_REFRESH_GRADE_TIMER] = function()
    event_team_ladder.on_refresh_grade_timer()
    return 0
end

g_func_map[TEAM_CHAMPION_FORECAST_ENROLL_END_TIMER] = function( _zone_id )
    event_team_champion.on_forecast_enroll_end( _zone_id )
    return 0
end

g_func_map[TEAM_CHAMPION_ENROLL_TIME_UP] = function( _zone_id )
    event_team_champion.on_enroll_time_up( _zone_id )
    return 0
end

g_func_map[TEAM_CHAMPION_REST_TIME_UP] = function( _zone_id )
    event_team_champion.on_rest_time_up( _zone_id )
    return 0
end

g_func_map[TEAM_CHAMPION_WAIT_PLAYER_JOIN_BATTLE_TIME_UP] = function( _inst_index )
    event_team_champion.on_wait_player_join_battle_time_up( _inst_index )
    return 0 
end

g_func_map[TEAM_CHAMPION_ONE_GAME_BATTLE_TIME_UP] = function( _inst_index )
    event_team_champion.on_one_game_time_up( _inst_index )
    return 0
end

g_func_map[TEAM_CHAMPION_READY_GO_TIME_UP] = function( _inst_index )
    event_team_champion.on_ready_go_time_up( _inst_index )
    return 0 
end

g_func_map[TEAM_CHAMPION_KICK_PLAYER_TIMER] = function( _inst_index )
    event_team_champion.on_kick_all_players_timer( _inst_index )
    return 0 
end

g_func_map[TEAM_CHAMPION_SHOW_RESULT_TIME_UP] = function( _inst_index )
    event_team_champion.on_finish_show_battle_result( _inst_index )
    return 0
end

g_func_map[TEAM_CHAMPION_SELECT_MODEL_TIMEUP] = function( _inst_index )
    event_team_champion.on_select_model_timeup( _inst_index )
    return 0
end

g_func_map[TEAM_CHAMPION_WAIT_RESULT_TIME_UP] = function( _zone_id )
    event_team_champion.on_wait_battle_result_time_up( _zone_id )
    return 0
end

