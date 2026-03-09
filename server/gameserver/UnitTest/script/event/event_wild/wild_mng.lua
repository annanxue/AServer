module( "wild_mng", package.seeall )

local sformat, mrandom = string.format, math.random

local ENTER_WILD_MIN_LEVEL = 10

local function can_enter_wild( _player, _wild_type )
    local wild_cfg = WILDS[_wild_type]
    if _player.level_ < wild_cfg.LimitLevel then
        c_err("wild_mng(can_enter_wild), can not join wild, limit_level:%d, player_level:%d", wild_cfg.LimitLevel, _player.level_)
        return false
    end
    return true
end

function get_zone_id()
    local zone_id = game_zone_mng.get_zone_id()
    if g_gamesvr:c_get_allow_wild_cross_zone() then
        zone_id = -1 -- means cross zone
    end
    return zone_id
end

function join_wild( _player, _wild_type, _wild_id, _trans_id )
    if not can_enter_wild( _player, _wild_type ) then
        return
    end
    local server_id = _player.server_id_
    local player_id = _player.player_id_
    local zone_id = get_zone_id()
    bfclient.remote_call_center( "wild_mng.join_wild", server_id, player_id, _wild_type, zone_id, _wild_id )
end

function auto_join_wild( _player, _wild_type, _trans_id )
    if not can_enter_wild( _player, _wild_type ) then
        return
    end
    local server_id = _player.server_id_
    local player_id = _player.player_id_
    local zone_id = get_zone_id()
    bfclient.remote_call_center( "wild_mng.auto_join_wild", server_id, player_id, _wild_type, zone_id, _trans_id )
end

function send_player_to_wild( _player_id, _bf_id, _wild_type, _inst_id, _world_id, _plane_id, _x, _z, _angle_y, _camp, _trans_id )
    local player = player_mng.get_player_by_player_id( _player_id )
    if not player then
        c_err( "[wild_mng](send_player_to_wild) player not found, player_id: %d", _player_id )
        return
    end
    player:add_join_instance_succ( _inst_id )
    local extra_data = player.extra_data_for_bf_
    extra_data.goto_bf_event_id_ = share_const.GOTO_BF_FOR_WILD
    extra_data.wild_type_ = _wild_type
    extra_data.wild_camp_ = _camp 
    extra_data.wild_trans_id_ = _trans_id
    extra_data.wild_x_ = _x
    extra_data.wild_z_ = _z
    extra_data.wild_angle_y_ = _angle_y
    extra_data.old_camp_ = player.core_:c_get_camp()
    bfclient.send_player_to_bf( _player_id, _bf_id, _world_id, _plane_id, _x, _z, _angle_y )
end

function send_fail_msg_to_client( _player_id, _msg_id )
    local player = player_mng.get_player_by_player_id( _player_id )
    if player then
        player:add_defined_text( _msg_id )
    end
end

function check_open_first_wild()
    local center_bf_zone_info_loaded = bfclient.g_is_zone_info_loaded_on_bf[0]
    local game_zone_info_loaded = game_zone_mng.is_load_zone_info_finish()
    if center_bf_zone_info_loaded and game_zone_info_loaded then
        local zone_id = get_zone_id()
        for wild_type, wild_cfg in pairs( WILDS ) do
            c_log( "[wild_mng](check_open_first_wild) notify center bf to open the first wild, wild_type: %d, zone_id: %d", wild_type, zone_id )
            bfclient.remote_call_center( "wild_mng.check_open_new_wild", wild_type, zone_id )
        end
    end
end
