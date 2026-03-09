module( "wild_mng", package.seeall )

PREPARE_PERIOD = 20 * g_timermng:c_get_frame_rate() -- 20秒
g_prepare_ = g_prepare_ or {}

function prepare_enter_guild_inst( _server_id, _player_id, _wild_id, _bf_id, _trans_id )
    g_prepare_[_player_id] = {_wild_id, _trans_id}
    g_timermng:c_add_timer( PREPARE_PERIOD, timers.PREPARE_GUILD_INST, _player_id, 0, 0 )
    bfclient.remote_call_bf( _bf_id, "wild_mng.on_prepare_enter_guild_inst_done", _server_id, _player_id )
    c_log("[wild_mng_guild](prepare_enter_guild_inst) server_id: %d , player_id: %d , wild_id: %d , bf_id: %d , trans_id: %d", _server_id, _player_id, _wild_id, _bf_id , _trans_id )
end

function before_player_join( _player )
    local wild_id, trans_id = check_swith_to_bf( _player )
    if wild_id == 0 then return end
    _player.switching_to_bf_ = true
end

function after_player_join( _player )
    local wild_id, trans_id = check_swith_to_bf( _player, true )
    if wild_id == 0 then return end
    wild_mng.auto_join_wild( _player, wild_id, trans_id )
end

--[[
function after_add_hero( _player )
    local wild_id, trans_id = check_swith_to_bf( _player, true )
    if wild_id == 0 then return end
    wild_mng.auto_join_wild( _player, wild_id, trans_id )
end
--]]

function check_swith_to_bf( _player, _is_clear )
    local player_id = _player.player_id_
    local info = g_prepare_[player_id]
    if _is_clear then
        g_prepare_[player_id] = nil
    end

    if _player:is_evil() and not _player:is_in_lucius_wild() then
        return share_const.LUCIUS_WILD_TYPE, 0
    end

    if not info then 
        return 0, 0
    end

    local wild_id, trans_id = info[1], info[2]
    return wild_id, trans_id
end

function prepare_timeout( _player_id )
    g_prepare_[_player_id] = nil
    c_log("[wild_mng_guild](prepare_timeout) player_id: %d", _player_id )
end

function test_guild_inst( _wild_id )
    local wild_id = _wild_id or 3
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do   
        if target:is_player() then 
            wild_mng.auto_join_wild( target, wild_id )
        end
    end
end
