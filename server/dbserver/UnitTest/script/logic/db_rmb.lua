module( "db_rmb", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------
local t_insert, s_format = table.insert, string.format

local SELECT_RMB_SQL = [[
    SELECT 
    `recharge_total`,
    `recharge_today`,
    `daily_first_time_recharge`, 
    `recharge_reward_diamond_list`, 
    `wittryaknight`,
    `leona_gift_record`,
    `first_recharge_award`,
    `competitive_halo`,
    `recharge_remind`,
    `buy_gold_cnt`,
    `award_consume_list`,
    `recharge_total_base_diamond`
    from PLAYER_RMB_TBL where player_id = ?;
]]

local SELECT_RMB_RESULT_TYPE = "iiissssssisi"

local MODIFY_RMB_SQL = [[
    REPLACE INTO PLAYER_RMB_TBL 
    ( `player_id`, 
      `recharge_total`, 
      `recharge_today`, 
      `daily_first_time_recharge`, 
      `recharge_reward_diamond_list`,
      `wittryaknight` , 
      `leona_gift_record`, 
      `first_recharge_award`,
      `competitive_halo`,
      `recharge_remind`,
      `buy_gold_cnt`, 
      `award_consume_list`,
      `recharge_total_base_diamond`)
      VALUES
    ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? );
]]

local MODIFY_RMB_PARAM_TYPE = "iiiissssssisi"

local INSERT_RMB_SQL = [[
    INSERT INTO PLAYER_RMB_TBL
    ( `player_id`, `recharge_reward_diamond_list`, `wittryaknight`, `leona_gift_record`, `first_recharge_award`, `competitive_halo`, `recharge_remind`,`award_consume_list`, `recharge_total_base_diamond`  )
    VALUES ( ?, "do local ret = {} return ret end", "do return {} end", "{}", "{}", "", "", "", "do local ret = {} return ret end" );
]]

local INSERT_RMB_PARAM_TYPE = "i"

local SAVE_RECHARGE_TOTAL_SQL = [[
    UPDATE PLAYER_RMB_TBL set recharge_total = ? where player_id = ?;
]]

local SAVE_RECHARGE_TOTAL_PARAM_TYPE = "ii"

local SAVE_RECHARGE_TODAY_SQL = [[
    UPDATE PLAYER_RMB_TBL set recharge_today = ? where player_id = ?;
]]

local SAVE_RECHARGE_TODAY_PARAM_TYPE = "ii"

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

local function get_init_rmb_record()
    local init_record = 
    { 
        recharge_total_ = 0, 
        recharge_today_ = 0, 
        daily_first_time_recharge_ = 0,
        recharge_reward_diamond_list_ = "do local ret = {} return ret end",
        wittryaknight_ = "do return {} end",
        leona_gift_record_ = "{}", 
        first_recharge_award_ = "{}",
        competitive_halo_ = "",
        recharge_remind_ = "",
        buy_gold_cnt_ = 0,
        award_consume_list_ = "",
        recharge_total_base_diamond_ = 0,
    }
    return init_record
end

function do_get_player_rmb( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_RMB_SQL, "i", _player_id, SELECT_RMB_RESULT_TYPE ) < 0 then
        c_err( "[db_rmb](do_get_player_rmb) c_stmt_format_query on SELECT_RMB_SQL failed, player_id: %d", _player_id )
        return
    end
    
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        if database:c_stmt_format_modify( INSERT_RMB_SQL, INSERT_RMB_PARAM_TYPE, _player_id ) < 0 then
            c_err( "[db_rmb](do_get_player_rmb) c_stmt_format_modify on INSERT_RMB_SQL failed, player_id: %d", _player_id )
        end
        
        return get_init_rmb_record() 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local total, 
                  today, 
                  daily_first_time_recharge, 
                  recharge_reward_diamond_list, 
                  leona_gift_record, 
                  first_recharge_award, 
                  wittryaknight,
                  competitive_halo,
                  recharge_remind,
                  buy_gold_cnt, 
                  award_consume_list,
                  recharge_total_base_diamond
                  = database:c_stmt_get( "recharge_total", 
                                         "recharge_today", 
                                         "daily_first_time_recharge",
                                         "recharge_reward_diamond_list", 
                                         "leona_gift_record", 
                                         "first_recharge_award", 
                                         "wittryaknight",
                                         "competitive_halo",
                                         "recharge_remind",
                                         "buy_gold_cnt",
                                         "award_consume_list",
                                         "recharge_total_base_diamond")
            return { 
                    recharge_total_ = total, 
                    recharge_today_ = today, 
                    daily_first_time_recharge_ = daily_first_time_recharge, 
                    recharge_reward_diamond_list_ = recharge_reward_diamond_list, 
                    wittryaknight_ = wittryaknight, 
                    leona_gift_record_ = leona_gift_record, 
                    first_recharge_award_ = first_recharge_award, 
                    competitive_halo_ = competitive_halo,
                    recharge_remind_ = recharge_remind,
                    buy_gold_cnt_ = buy_gold_cnt,
                    award_consume_list_ = award_consume_list,
                    recharge_total_base_diamond_ = recharge_total_base_diamond,
                }
        end
    end
end

function do_save_player_rmb( _player )
    local player_id = _player.player_id_
    local rmb = _player.rmb_
    local total = rmb.recharge_total_
    local today = rmb.recharge_today_
    local reward_diamond_list = rmb.recharge_reward_diamond_list_
    local daily_first_time_recharge = rmb.daily_first_time_recharge_

    local wittryaknight = rmb.wittryaknight_ 
    local leona_gift_record = rmb.leona_gift_record_
    local first_recharge_award = rmb.first_recharge_award_
    local competitive_halo = rmb.competitive_halo_
    local recharge_remind = rmb.recharge_remind_
    local buy_gold_cnt = rmb.buy_gold_cnt_
    local award_consume_list = rmb.award_consume_list_

    local recharge_total_base_diamond = rmb.recharge_total_base_diamond_

    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_RMB_SQL, MODIFY_RMB_PARAM_TYPE,
                                                         player_id, 
                                                         total, 
                                                         today, 
                                                         daily_first_time_recharge, 
                                                         reward_diamond_list, 
                                                         wittryaknight, 
                                                         leona_gift_record, 
                                                         first_recharge_award,
                                                         competitive_halo,
                                                         recharge_remind,
                                                         buy_gold_cnt,
                                                         award_consume_list,
                                                         recharge_total_base_diamond)
end

function save_recharge_total_realtime( _player_id, _recharge_total )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( SAVE_RECHARGE_TOTAL_SQL, SAVE_RECHARGE_TOTAL_PARAM_TYPE, _recharge_total, _player_id ) < 0 then
        c_err( "[db_rmb](save_recharge_total_realtime) player_id: %d failed to save recharge total: %d", _player_id, _recharge_total )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_rmb](save_recharge_total_realtime)player_id: %d recharge_total: %d use time: %d", _player_id, _recharge_total, time )
    return true
end

function save_recharge_today_realtime( _player_id, _recharge_today )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( SAVE_RECHARGE_TODAY_SQL, SAVE_RECHARGE_TODAY_PARAM_TYPE, _recharge_today, _player_id ) < 0 then
        c_err( "[db_rmb](save_recharge_today_realtime) player_id: %d failed to save recharge today: %d", _player_id, _recharge_today )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_rmb](save_recharge_today_realtime)player_id: %d recharge_today: %d use time: %d", _player_id, _recharge_today, time )
    return true
end

