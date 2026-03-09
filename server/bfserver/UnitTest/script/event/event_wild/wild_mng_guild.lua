module( "wild_mng", package.seeall )

local tinsert = table.insert 
local mrandom = math.random
local get_ctrl, get_plane, get_inst, get_inst_by_index =  ctrl_mng.get_ctrl, planemng.get_plane, instance_mng.get_plane_instance, instance_mng.get_instance

TYPE_TO_KILL_PLAYER_POINT = 
{
    [3] = GUILDPROF_TASK_SYS_CFG.FightPoint[1], 
    [4] = GUILDPROF_TASK_SYS_CFG.ExorcistPoint[1], 
}


self_bf_id = g_bfsvr:c_get_bf_id()

local KILL_TYPE_NORMAL        = 0x0001  -- 普通击杀

local function rand_inst_alive_player( _D )
    local inst = _D:get_instance()
    if not inst then
        return -1
    end
    local list = {}
    local except_id = _D.ctrl_id_
    for ctrl_id, _ in pairs(inst.player_list_) do
        if ctrl_id ~= except_id then
            local obj = get_ctrl( ctrl_id )
            if obj and obj.core_:c_is_state( states.STATE_DEAD ) == 0 then
                tinsert( list, ctrl_id )
            end
        end
    end
    local len = #list
    if len == 0 then
        return -1
    end
    local rand = mrandom(1,len)
    local rand_id = list[rand]
    return rand_id
end

local function combo_kill_rule( _D, _A, _revive_sec )
    local kill_type = 0
    if _A and _A:is_player() and _A.ctrl_id_ ~= _D.ctrl_id_ then
        kill_type = c_bor( 0, KILL_TYPE_NORMAL )
    end

    local rand_id = rand_inst_alive_player( _D )
    player_mng.broadcast_kill_type( _A, _D, kill_type, _revive_sec, rand_id)
end

local function get_kill_player_point( _wild_type )
    return TYPE_TO_KILL_PLAYER_POINT[_wild_type] or 0
end

local function get_kill_exorcist_point( _wild_type, _player )
    if _wild_type ~= 4 then 
        return 0
    end
    local pre_timer = _player.exorcist_pre_timer_ or 0
    local old_timer = _player.exorcist_timer_ or 0
    if pre_timer == 0 and old_timer == 0 then
        return 0
    end
    return GUILDPROF_TASK_SYS_CFG.KillExorcistPoint[1] or 0
end

function on_dead_in_guild_inst( _D, _A )
    local plane = planemng.get_player_plane( _D )
    local wild_type = plane.wild_type_
    local wild_cfg = WILDS[wild_type]

    if _D and _D:is_player( ) and _D:need_record_damage_player( ) then
        local inst = _A:get_instance()
        if inst then
            local time_damage = _D:get_time_damage( )
            for ctrl_id, _ in pairs( time_damage ) do
                local attacker = ctrl_mng.get_ctrl( ctrl_id )
                if attacker and attacker:is_player() and attacker.ctrl_id_ ~= _D.ctrl_id_ then
                    -- 下面函数顺序不能换哦
                    local in_give_reward_cd = player_t.get_is_in_kill_reward_cd(attacker, _D, inst)
                    player_t.on_kill_player_update_kill_record( attacker, _D, inst )
                    -- 1. kill player add point
                    if not in_give_reward_cd then
                        local monster_type = -1 -- 规定好了，玩家填-1, 假装是一个怪，因为任务目标是击杀某些种类怪得士气
                        local point = get_kill_player_point(wild_type)
                        local exorcist_point = get_kill_exorcist_point(wild_type, _D)
                        point = (exorcist_point > 0) and exorcist_point or point
                        if point > 0 then
                            quest_t.quest_event_notify( attacker.player_id_, target_type.TARGET_TYPE_GUILDPROF_POINT, {monster_type, point}, attacker)
                        end
                    end
                end
            end

        end
    end

    combo_kill_rule(_D, _A, wild_cfg.ReviveCD)

    if wild_type == 4 then
        -- 猎魔人
        recover_from_exorcist( _D.server_id_, _D.player_id_ )
    end
    
end

function on_add_player_in_guild_inst( _player )
    local inst = _player:get_instance()
    _player.record_damage_player_ = GUILDPROF_TASK_SYS_CFG.RecordDamagePlayer[1] --记录死前5s内造成伤害的玩家
    if inst then
        _player:on_inst_add_player_for_prevent_kill_cheat( inst )
    end
end

function on_player_before_leave_in_guild_inst( _player )
    local inst = _player:get_instance()
    _player.record_damage_player_ = nil
    if inst then
        _player:on_inst_remove_player_for_prevent_kill_cheat( inst )
    end
    local plane = planemng.get_player_plane( _player )
    local wild_type = plane.wild_type_
    if wild_type == 4 then
        -- 猎魔人
        recover_from_exorcist( _player.server_id_, _player.player_id_ )
    end
end

local function check_wild_trans_limit( _player, _wild_id , _trans_id )
    local trans_cfg = WILDS_TRANSFER[_trans_id]
    if not trans_cfg then
        return -1
    end
    local prof_limit = trans_cfg.GuildProfLimit
    if prof_limit >0 then
        local prof_id, _ = _player:get_on_job_prof_id_level()
        if prof_limit ~= prof_id then
            return -2
        end
    end

    --[[
    -- 2017-01-19有猎魔人的buff也能退出
    if ( _player.exorcist_timer_ or 0 ) > 0 then
        local alert_text = MESSAGES["GUILDPROF_EXORCIST_CANNOT_LEAVE_WHILE_CHANGING"]
        _player:add_defined_text(alert_text) 
        return 3
    end
    --]]

    return 0
end

function on_auto_wild_transfer( _player, _wild_id, _trans_id,  _is_test )
    local check_ret = check_wild_trans_limit(_player, _wild_id, _trans_id)
    if (check_ret ~= 0) and (not _is_test) then
        if check_ret < 0 then
            c_err("[wild_mng_guild](on_auto_wild_transfer) check fail, pid:%d , wild_id:%d , trans_id: %d , reason: %d", _player.player_id_, _wild_id, _trans_id, check_ret )
        end
        return
    end

    -- 先退回GS
    local server_id = _player.server_id_
    local player_id = _player.player_id_
    bfsvr.remote_call_game( server_id, "wild_mng.prepare_enter_guild_inst", server_id, player_id, _wild_id, self_bf_id, _trans_id )
    c_log("[wild_mng_guild](on_auto_wild_transfer) server_id: %d , player_id: %d , wild_id: %d , trans_id: %d", server_id, player_id, _wild_id, _trans_id )
end

function on_prepare_enter_guild_inst_done( _server_id, _player_id )
    local player = player_mng.get_player( _server_id, _player_id )
    if not player then
        return
    end
    bfsvr.send_player_back_to_game( player )
    c_log("[wild_mng_guild](on_prepare_enter_guild_inst_done) server_id: %d , player_id: %d", _server_id, _player_id )
end











local function restore_player( _player )
    _player:set_camp( _player.old_camp_ )
    _player.on_dead = _player.old_on_dead_
    _player.before_leave = _player.old_before_leave_
    _player.pnum_ = nil
    _player:del_plane_buffs()
    _player:restore_full_hp()
end


function test_guild_inst( _wild_id )
    local wild_id = _wild_id or 3
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do   
        if target:is_player() then 
            on_auto_wild_transfer( target, wild_id, 0,  true )
        end
    end
end

function print_camp( )
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do   
        if target:is_player() then 
            local camp = target.core_:c_get_camp()
            c_err("print_camp id: %d , camp: %d", id, camp)
        end
    end
end

function recover_from_exorcist( _server_id, _player_id )
    local player = player_mng.get_player( _server_id, _player_id )
    if not player then
        return
    end
    local inst = player:get_instance()
    if not inst then
        return
    end
    local pre_timer = player.exorcist_pre_timer_ or 0
    if pre_timer > 0 then
        g_timermng:c_del_timer( pre_timer )
        player.exorcist_pre_timer_ = nil
        inst:add_exorcist_cnt(-1)
        return
    end

    local old_timer = player.exorcist_timer_ or 0
    if old_timer == 0 then
        return
    end
    g_timermng:c_del_timer( old_timer )
    player.exorcist_timer_ = nil
    player:set_camp( GUILDPROF_TASK_SYS_CFG.ExorcistDefaultCamp[1], true )
    inst:add_exorcist_cnt(-1)

    local self_text = MESSAGES[GUILDPROF_TASK_SYS_CFG.ExorcistEnd[1]]
    local other_text = MESSAGES[GUILDPROF_TASK_SYS_CFG.ExorcistDisappear[1]]
    local self_ctrl_id = player.ctrl_id_
    player:add_defined_text( self_text )

    for ctrl_id, _ in pairs( inst.player_list_ ) do
        if ctrl_id ~= self_ctrl_id then
            local obj = ctrl_mng.get_ctrl(ctrl_id)
            if obj then
                obj:add_defined_text( other_text )
            end
        end
    end
end

function on_exorcist_pre_change_timer( _inst_idx, _self_ctrl_id )
    local inst = instance_mng.get_instance( _inst_idx )
    if not inst then
        return
    end
    local exorcist_change = inst.exorcist_change_
    if not exorcist_change then
        inst:add_exorcist_cnt(-1)
        return
    end
    local info = exorcist_change[_self_ctrl_id]
    if not info then
        inst:add_exorcist_cnt(-1)
        return
    end

    local player = ctrl_mng.get_ctrl( _self_ctrl_id )
    if not( player and player:is_player() )then
        inst:add_exorcist_cnt(-1)
        return
    end
    player.exorcist_pre_timer_ = nil

    local buff_id = info.buff_id_
    local change_camp = info.change_camp_
    local change_time = info.change_time_
    player:do_buff(buff_id, _self_ctrl_id)
    player:set_camp( change_camp, true )
    local old_timer = player.exorcist_timer_ or 0
    if old_timer > 0 then
        g_timermng:c_del_timer( old_timer )
    end
    player.exorcist_timer_ = g_timermng:c_add_timer_second( change_time, timers.GUILDPROF_EXORCIST_CHANGE, player.server_id_, player.player_id_, 0 )
    local self_text = MESSAGES[GUILDPROF_TASK_SYS_CFG.ExorcistStart[1]]
    local other_text = MESSAGES[GUILDPROF_TASK_SYS_CFG.ExorcistAppear[1]]
    player:add_defined_text( self_text )
    for ctrl_id, _ in pairs( inst.player_list_ ) do
        if ctrl_id ~= _self_ctrl_id then
            local obj = ctrl_mng.get_ctrl(ctrl_id)
            if obj then
                obj:add_defined_text( other_text )
            end
        end
    end

end




