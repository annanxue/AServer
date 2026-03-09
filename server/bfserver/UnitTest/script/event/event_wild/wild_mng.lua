module( "wild_mng", package.seeall )

local sformat, next, tinsert, tsort, ceil, tremove = string.format, next, table.insert, table.sort, math.ceil, table.remove
local mrandom, msin, mcos, mrad = math.random, math.sin, math.cos, math.rad
local ostime = os.time
local mfloor = math.floor
local LOG_HEAD = "[wild_mng]"

local WILD_NUM_PER_PAGE = 5 -- 跟客户端UI对应
local frame_rate = g_timermng:c_get_frame_rate()

GUILD_FIGHT_LEFT_CAMP = 16
GUILD_FIGHT_RIGHT_CAMP = 17



CHECK_PERIOD = frame_rate * 5
CAN_ENTER_CHECK_TIME = 60*5
WILD_CHECK_TIMEOUT = CAN_ENTER_CHECK_TIME + 60

self_bf_id = g_bfsvr:c_get_bf_id()
-- bf0
if self_bf_id == 0 then
    g_wild_mng = g_wild_mng or {}
    g_id_list = g_id_list or {}
    g_joining_map = g_joining_map or {}
    g_joining_sn = g_joining_sn or 1
    g_check_timer = g_check_timer or 0
    if g_check_timer > 0 then
        g_timermng:c_del_timer( g_check_timer )
    end
    local rand = mrandom(5,10)
    g_check_timer = g_timermng:c_add_timer( rand, timers.CHECK_OPENED_WILDS, 0, 0, CHECK_PERIOD )
    g_unique_id = g_unique_id or 0
else
    -- bf 1~n
    g_unique_map = g_unique_map or {}
end



local function get_id_list( _wild_type, _zone_id )
    local type_idlist = g_id_list[_wild_type]
    if not type_idlist then
        type_idlist = {}
        g_id_list[_wild_type] = type_idlist
    end
    local zone_idlist = type_idlist[_zone_id]
    if not zone_idlist then
        zone_idlist = {}
        type_idlist[_zone_id] = zone_idlist
    end
    return zone_idlist
end

local function add_to_id_list( _wild_type, _zone_id, _wild_id )
    local id_list = get_id_list( _wild_type, _zone_id )
    tinsert( id_list, _wild_id )
    tsort( id_list )
end

local function remove_from_id_list( _wild_type, _zone_id, _wild_id )
    local id_list = get_id_list( _wild_type, _zone_id )
    local target_index
    for i, wild_id in ipairs( id_list ) do
        if wild_id == _wild_id then
            target_index = i
        end
    end
    if target_index then
        tremove( id_list, target_index )
    else
        c_err( "[wild_mng](remove_from_id_list) wild_id not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
    end
end

local function get_zone_wilds( _wild_type, _zone_id )
    local type_wilds = g_wild_mng[_wild_type]
    if not type_wilds then
        type_wilds = {}
        g_wild_mng[_wild_type] = type_wilds
    end
    local zone_wilds = type_wilds[_zone_id]
    if not zone_wilds then
        zone_wilds = {}
        type_wilds[_zone_id] = zone_wilds
    end
    return zone_wilds
end

local function get_wild( _wild_type, _zone_id, _wild_id )
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    return zone_wilds[_wild_id]
end

local function get_wild_bf_id( _wild_id )
    local bf_count = g_bfsvr:c_get_bf_num()
    local bf_id = ( _wild_id - 1 ) % (bf_count - 1)
    return bf_id + 1
end

local function get_next_wild_id( _wild_type, _zone_id )
    local wild_cfg = WILDS[_wild_type]
    local max_wild_num = wild_cfg.MaxWildNum
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    for i = 1, max_wild_num, 1 do
        if not zone_wilds[i] then
            return i
        end
    end
    return max_wild_num + 1
end

local function get_next_unique_id()
    g_unique_id = g_unique_id + 1
    return g_unique_id
end

function leave_wild( _wild_type, _zone_id, _wild_id, _camp, _server_id, _player_id )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](leave_wild) wild not found, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id )
        return
    end

    local player_map = wild.player_map_[_server_id]
    if not player_map then
        c_err( "[wild_mng](leave_wild) player map not found, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )
        return
    end

    if not player_map[_player_id] then
        c_err( "[wild_mng](leave_wild) player not in wild, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )
        return
    end

    player_map[_player_id] = nil
    wild.player_num_ = wild.player_num_ - 1
    c_log( "[wild_mng](leave_wild) player leave, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
        _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )

    if _camp ~= PLAYER_ATTR[1].Camp then
        if not _camp then
            _camp = GUILD_FIGHT_LEFT_CAMP
        end
        local camp_pcnt = wild.camp_pcnt_
        camp_pcnt[_camp] = (camp_pcnt[_camp] or 0) - 1
    end
    
    if wild.player_num_ <= 0 then
        check_close_empty_wild( _wild_type, _zone_id, _wild_id )
    end
end

function check_open_new_wild( _wild_type, _zone_id )
    local wild_cfg = WILDS[_wild_type]
    if not wild_cfg then
        c_err( "[wild_mng](check_open_new_wild) wild cfg not found, wild_type: %d", _wild_type )
        return
    end
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    for wild_id, wild in pairs( zone_wilds ) do
        if wild.player_num_ < wild_cfg.PlayerNumToOpenNewWild then
            c_log( "[wild_mng](check_open_new_wild) no need to open new wild, wild_type: %d, zone_id: %d, wild_id: %d, player_num: %d", _wild_type, _zone_id, wild_id, wild.player_num_ )
            return
        end
    end
    c_log( "[wild_mng](check_open_new_wild) all wilds player_num reach limit, open new wild, wild_type: %d, zone_id: %d", _wild_type, _zone_id )
    open_new_wild( _wild_type, _zone_id )
end

function open_new_wild( _wild_type, _zone_id )
    local wild_cfg = WILDS[_wild_type]
    local wild_id = get_next_wild_id( _wild_type, _zone_id )
    if wild_id > wild_cfg.MaxWildNum then
        c_log( "[wild_mng](open_new_wild) wild num reach max, wild_type: %d, zone_id: %d", _wild_type, _zone_id )
        return
    end
    c_log( "[wild_mng](open_new_wild) open new wild, wild_type: %d, zone_id: %d", _wild_type, _zone_id )

    local unique_id = get_next_unique_id()
    local wild = 
    {
        unique_id_      = unique_id,
        wild_id_        = wild_id,
        wild_type_      = _wild_type,
        inst_index_     = 0,
        joining_sn_map_ = {},
        player_map_     = {},
        player_num_     = 0,
        can_enter_      = false,
        check_time_     = 0,
        is_has_boss_    = 0,
        camp_pcnt_ = {
            [GUILD_FIGHT_LEFT_CAMP] = 0,
            [GUILD_FIGHT_RIGHT_CAMP] = 0,
        },
    }
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    zone_wilds[wild_id] = wild
    add_to_id_list( _wild_type, _zone_id, wild_id )

    local bf_id = get_wild_bf_id( wild_id )
    bfsvr.remote_call_bf( bf_id, "wild_mng.create_wild_instance", _wild_type, _zone_id, wild_id, unique_id )
end

local function foreach_wild( _func )
    for wild_type, type_wilds in pairs( g_wild_mng ) do
        for zone_id, zone_wilds in pairs( type_wilds ) do
            for wild_id, wild in pairs( zone_wilds ) do
                _func( wild_type, zone_id, wild_id, wild )
            end
        end
    end
end

function check_one_wild( _wild_type, _zone_id, _wild_id, _wild )
    if not _wild.can_enter_ then
        -- 之前没创建成功, 所以继续尝试创建
        local bf_id = get_wild_bf_id( _wild_id )
        bfsvr.remote_call_bf( bf_id, "wild_mng.create_wild_instance", _wild_type, _zone_id, _wild_id, _wild.unique_id_ )
        c_log("check_one_wild recreate wild_type:%d, zone_id:%d, wild_id:%d, unique_id:%d", _wild_type, _zone_id, _wild_id, _wild.unique_id_)
        return
    end

    -- 检查副本是否还存在
    local check_time = _wild.check_time_
    local cur_time = ostime()
    local diff_time = cur_time - check_time
    if diff_time > WILD_CHECK_TIMEOUT then
        _wild.can_enter_ = false
        check_one_wild( _wild_type, _zone_id, _wild_id, _wild )
        return
    elseif diff_time < CAN_ENTER_CHECK_TIME then
        -- 能进的副本5分钟检查一次状态
        return
    end

    -- 当大于5分钟没有返回结果时,每5秒检查一遍
    local bf_id = get_wild_bf_id( _wild_id )
    bfsvr.remote_call_bf( bf_id, "wild_mng.check_wild_instance", _wild.unique_id_ )
    c_log("check_one_wild one_check wild_type:%d, zone_id:%d, wild_id:%d, unique_id:%d", _wild_type, _zone_id, _wild_id, _wild.unique_id_)
end

function check_opened_wilds()
    foreach_wild( check_one_wild )
    return 1
end

function check_wild_instance( _unique_id )
    local inst_index = g_unique_map[_unique_id]
    if not inst_index then return end 

    local inst = instance_mng.get_instance( inst_index )
    if not inst then return end

    local plane = inst.plane_ 
    local wild_type = plane.wild_type_
    local zone_id = plane.zone_id_
    local wild_id = plane.wild_id_

    bfsvr.remote_call_center( "wild_mng.set_wild_instance_id", wild_type, zone_id, wild_id, inst_index )
    c_log("[wild_mng](check_wild_instance) ok wild_type:%d, zone_id:%d, wild_id:%d, inst_index:%d, unique_id:%d", wild_type, zone_id, wild_id, inst_index, _unique_id)
end

function create_wild_instance( _wild_type, _zone_id, _wild_id, _unique_id )
    if g_unique_map[_unique_id] then
        local inst_index = g_unique_map[_unique_id]
        bfsvr.remote_call_center( "wild_mng.set_wild_instance_id", _wild_type, _zone_id, _wild_id, inst_index )
        c_log( "[wild_mng](create_wild_instance) already exist, wild_type: %d, zone_id: %d, wild_id: %d, inst_index: %d, unique_id:%d", _wild_type, _zone_id, _wild_id, inst_index, _unique_id )
        return
    end

    local wild_cfg = WILDS[_wild_type]
    if not wild_cfg then
        c_err( "[wild_mng](create_wild_instance) wild cfg not found, wild_type: %d", _wild_type )
        return
    end
    local act_inst_id = wild_cfg.ActInstID
    local act_inst_cfg = ACTIVITY_INSTANCE_CONFIG[act_inst_id]
    if not act_inst_cfg then
        c_err( "[wild_mng](create_wild_instance) act inst cfg not found, wild_type: %d, act_inst_id: %d", _wild_type, act_inst_id )
        return
    end
    local inst_id = act_inst_cfg.InstanceId
    local inst = instance_mng.create_instance( inst_id )
    local plane = inst.plane_
    plane.wild_type_ = _wild_type
    plane.zone_id_ = _zone_id
    plane.wild_id_ = _wild_id
    plane.unique_id_ = _unique_id
    plane.boss_num_ = 0
    inst:init_scene_spawn( inst_id )
    inst:set_allow_drop( false )

    local old_on_add_obj = inst.on_add_obj
    inst.on_add_obj = function ( self, _objid )
        old_on_add_obj( self, _objid )
        on_instance_add_obj( self, _wild_type, _zone_id, _wild_id, _objid )
    end

    local old_on_remove_obj = inst.on_remove_obj
    inst.on_remove_obj = function ( self, _objid, _change_instance )
        old_on_remove_obj( self, _objid, _change_instance )
        on_instance_remove_obj( self, _wild_type, _zone_id, _wild_id, _objid )
    end

    -- pick point 
    local inst_index = inst.index_
    local refresh_cfg_id_set = wild_cfg.PickPointRefreshConfigIDSet
    if refresh_cfg_id_set then
        for _, refresh_cfg_id in pairs( refresh_cfg_id_set ) do
            local sn = pick_point_mng.start_refresh( refresh_cfg_id, inst.world_id_, inst.plane_id_ )
            if sn then
                inst.pick_point_mng_sn_list_ = inst.pick_point_mng_sn_list_ or {}
                tinsert(inst.pick_point_mng_sn_list_, sn)
            end
        end
    end


    c_log( "[wild_mng](create_wild_instance) instance created, wild_type: %d, zone_id: %d, wild_id: %d, inst_index: %d, unique_id:%d", _wild_type, _zone_id, _wild_id, inst_index, _unique_id )

    g_unique_map[_unique_id] = inst_index
    bfsvr.remote_call_center( "wild_mng.set_wild_instance_id", _wild_type, _zone_id, _wild_id, inst_index )
end

function set_wild_instance_id( _wild_type, _zone_id, _wild_id, _inst_index )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](set_wild_instance_id) wild not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
        return
    end
    wild.inst_index_ = _inst_index
    wild.can_enter_ = true
    wild.check_time_ = ostime()
    c_log( "[wild_mng](set_wild_instance_id) wild_type: %d, zone_id: %d, wild_id: %d, inst_index: %d", _wild_type, _zone_id, _wild_id, _inst_index )
end

function check_close_empty_wild( _wild_type, _zone_id, _wild_id )
    local wild_cfg = WILDS[_wild_type]
    local player_num_to_open_new_wild = wild_cfg.PlayerNumToOpenNewWild
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id ) 
    for wild_id, wild in pairs( zone_wilds ) do
        if wild_id ~= _wild_id and wild.player_num_ < player_num_to_open_new_wild then
            c_log( "[wild_mng](check_close_empty_wild) still have other wilds, close this one, wild_type: %d, zone_id: %d, wild_id: %d, other_wild_id: %d", _wild_type, _zone_id, _wild_id, wild_id )
            close_empty_wild( _wild_type, _zone_id, _wild_id )
            return
        end
    end
    c_log( "[wild_mng](check_close_empty_wild) do not close the last wild, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
end

function close_empty_wild( _wild_type, _zone_id, _wild_id )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](close_empty_wild) wild not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
        return
    end
    if wild.player_num_ > 0 then
        c_err( "[wild_mng](close_empty_wild) wild is not empty, wild_type: %d, zone_id: %d, wild_id: %d, player_num: %d", _wild_type, _zone_id, _wild_id, wild.player_num_ )
        return
    end

    local inst_index = wild.inst_index_
    local bf_id = get_wild_bf_id( _wild_id )
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    zone_wilds[_wild_id] = nil
    remove_from_id_list( _wild_type, _zone_id, _wild_id )
    c_log( "[wild_mng](close_empty_wild) close empty wild, wild_type: %d, zone_id: %d, wild_id: %d, inst_index: %d", _wild_type, _zone_id, _wild_id, inst_index )
    bfsvr.remote_call_bf( bf_id, "wild_mng.delete_wild_instance", _wild_type, _zone_id, _wild_id, inst_index )
end

function delete_wild_instance( _wild_type, _zone_id, _wild_id, _inst_index )
    local inst = instance_mng.get_instance( _inst_index )
    if not inst then
        c_err( "[wild_mng](delete_wild_instance) instance not found, wild_type: %d, zond_id: %d, wild_id: %d, inst_index: %d", _wild_type, _zone_id, _wild_id, _inst_index )
        return
    end
    if next( inst.player_list_ ) then
        c_err( "[wild_mng](delete_wild_instance) player in instance, wild_type: %d, zond_id: %d, wild_id: %d, inst_index: %d", _wild_type, _zone_id, _wild_id, _inst_index )
        return
    end
    local plane = inst.plane_
    local unique_id = plane.unique_id_

    local wild_cfg = WILDS[_wild_type]
    local pick_point_mng_sn_list = inst.pick_point_mng_sn_list_
    for _, sn in ipairs( pick_point_mng_sn_list or {} ) do
        pick_point_mng.stop_refresh( sn )
    end

    egg_mng.before_wild_inst_deleted( isnt )

    instance_mng.delete_instance( _inst_index )
    g_unique_map[unique_id] = nil
    c_log( "[wild_mng](delete_wild_instance) delete wild instance, wild_type: %d, zone_id: %d, wild_id: %d, inst_index: %d", _wild_type, _zone_id, _wild_id, _inst_index )
end

function auto_join_wild( _server_id, _player_id, _wild_type, _zone_id, _trans_id )
    local wild_cfg = WILDS[_wild_type]
    local max_player_num = wild_cfg.MaxPlayerNum
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    local target_id
    local target_player_num = -1
    for wild_id, wild in pairs( zone_wilds ) do
        local player_num = wild.player_num_
        if player_num > target_player_num and player_num < max_player_num then
            target_id = wild_id
            target_player_num = player_num
            max_player_num = player_num
        end
    end
    if target_id then
        join_wild( _server_id, _player_id, _wild_type, _zone_id, target_id, _trans_id )
    else
        local msg_id = MESSAGES.ALL_WILD_IS_FULL
        bfsvr.remote_call_game( _server_id, "wild_mng.send_fail_msg_to_client", _player_id, msg_id )
    end
end

function get_enter_camp( _wild, _wild_cfg )
    if _wild_cfg.CampDiv == 0 then
        return PLAYER_ATTR[1].Camp
    end
    local camp = nil
    local camp_pcnt = _wild.camp_pcnt_
    local left_camp_pcnt = camp_pcnt[GUILD_FIGHT_LEFT_CAMP]
    local right_camp_pcnt = camp_pcnt[GUILD_FIGHT_RIGHT_CAMP]
    if left_camp_pcnt > right_camp_pcnt then
        camp = GUILD_FIGHT_RIGHT_CAMP
    else
        camp = GUILD_FIGHT_LEFT_CAMP
    end
    camp_pcnt[camp] = camp_pcnt[camp] + 1
    return camp
end

function join_wild( _server_id, _player_id, _wild_type, _zone_id, _wild_id, _trans_id )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild or not wild.can_enter_ then
        local msg_id = MESSAGES.NO_SUCH_WILD
        bfsvr.remote_call_game( _server_id, "wild_mng.send_fail_msg_to_client", _player_id, msg_id )
        return
    end

    local wild_cfg = WILDS[_wild_type]
    if wild.player_num_ >= wild_cfg.MaxPlayerNum then
        local msg_id = MESSAGES.WILD_IS_FULL
        bfsvr.remote_call_game( _server_id, "wild_mng.send_fail_msg_to_client", _player_id, msg_id )
        return
    end

    local player_map = wild.player_map_[_server_id]
    if not player_map then
        player_map = {}
        wild.player_map_[_server_id] = player_map
    end

    if player_map[_player_id] then
        c_err( "[wild_mng](join_wild) player already in wild, wild_type: %d, zone_id: %d, wild_id: %d, trans_id: %d, server_id: %d, player_id: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _trans_id or 0, _server_id, _player_id, wild.player_num_ )
        return
    end

    -- increase player num
    wild.player_num_ = wild.player_num_ + 1 
    player_map[_player_id] = true

    -- record joining info
    local joining_sn = g_joining_sn
    local timer_id = g_timermng:c_add_timer_second( 60, timers.JOIN_WILD_TIMEOUT, joining_sn, 0, 0 )
    local camp = get_enter_camp( wild, wild_cfg )
    g_joining_map[joining_sn] = 
    {
        wild_type_ = _wild_type,
        zone_id_   = _zone_id,
        wild_id_   = _wild_id,
        server_id_ = _server_id,
        player_id_ = _player_id,
        timer_id_  = timer_id,
        camp_      = camp,
        trans_id_  = _trans_id,
    }
    g_joining_sn = g_joining_sn + 1

    local sn_map = wild.joining_sn_map_[_server_id]
    if not sn_map then
        sn_map = {}
        wild.joining_sn_map_[_server_id] = sn_map 
    end
    sn_map[_player_id] = joining_sn
    c_log( "[wild_mng](join_wild) player join, wild_type: %d, zone_id: %d, wild_id: %d, trans_id: %d, server_id: %d, player_id: %d, joining_sn: %d, player_num: %d",
        _wild_type, _zone_id, _wild_id, _trans_id or 0, _server_id, _player_id, joining_sn, wild.player_num_ )

    -- check open new wild
    if wild.player_num_ >= wild_cfg.PlayerNumToOpenNewWild then
        check_open_new_wild( _wild_type, _zone_id )
    end

    -- notify wild bf
    local bf_id = get_wild_bf_id( _wild_id )
    local inst_index = wild.inst_index_
    bfsvr.remote_call_bf( bf_id, "wild_mng.convene_player", _server_id, _player_id, _wild_type, inst_index, camp, _trans_id )
end

function get_spawn_pos( _wild_type, _camp, _world_id, _plane_id, _trans_id )
    local wild_cfg = WILDS[_wild_type]
    local trans_id = ( _trans_id ~= 0 ) and _trans_id or wild_cfg.DefaultTrans
    local trans_cfg = WILDS_TRANSFER[trans_id]
    if trans_cfg.DstPos ~= nil then
        local world_id = _world_id
        local plane_id = _plane_id
        local spawn_pos_list = trans_cfg.DstPos
        local pos_index = mrandom( #spawn_pos_list )
        local spawn_pos = spawn_pos_list[pos_index]
        local center_x = spawn_pos[1]
        local center_z = spawn_pos[2]
        local angle_y = mrandom( 360 )
        local radius = mrandom( spawn_pos[3] )
        local radians = mrad( mrandom( 360 ) )
        local sin = msin( radians )
        local cos = mcos( radians )
        local x = radius * cos + center_x
        local z = radius * sin + center_z
        local hit_ret, hit_x, hit_z, hit_rate = c_raycast( world_id, plane_id, center_x, center_z, x, z )
        return hit_x, hit_z, angle_y
    end

    local spawn_pos_list = trans_cfg.DstCampPos[_camp]
    local rand = mrandom(1,#spawn_pos_list)
    local spawn_pos = spawn_pos_list[rand]
    return spawn_pos[1], spawn_pos[2], spawn_pos[3]
end

function convene_player( _server_id, _player_id, _wild_type, _inst_index, _camp, _trans_id )
    local instance = instance_mng.get_instance( _inst_index )
    if not instance then
        c_err( "[wild_mng](do_join_wild) instance not found, inst_index: %d", _inst_index )
        return
    end
    local bf_id = g_bfsvr:c_get_bf_id()
    local inst_id = instance.inst_id_
    local world_id = instance.world_id_
    local plane_id = instance.plane_id_

    local hit_x, hit_z, angle_y = get_spawn_pos( _wild_type, _camp, world_id, plane_id, _trans_id  )
    bfsvr.remote_call_game( _server_id, "wild_mng.send_player_to_wild", _player_id, bf_id, _wild_type, inst_id, world_id, plane_id, hit_x, hit_z, angle_y, _camp, _trans_id )
end

function on_player_join_succ( _wild_type, _zone_id, _wild_id, _server_id, _player_id )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](on_player_join_succ) wild not found, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id )
        return
    end

    local sn_map = wild.joining_sn_map_[_server_id]
    if not sn_map then
        c_err( "[wild_mng](on_player_join_succ) joining map not found, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )
        return
    end
    

    -- join succ before timeout, just clear joining info and return
    local joining_sn = sn_map[_player_id]
    if joining_sn then
        sn_map[_player_id] = nil
        local joining_info = g_joining_map[joining_sn]
        if not joining_info then
            c_err( "[wild_mng](on_player_join_succ) joining info not found, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d, player_num: %d",
                _wild_type, _zone_id, _wild_id, _server_id, _player_id, joining_sn, wild.player_num_ )
            return
        end
        g_timermng:c_del_timer( joining_info.timer_id_ )
        g_joining_map[joining_sn] = nil
        c_log( "[wild_mng](on_player_join_succ) join wild before timeout, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id, joining_sn, wild.player_num_ )
        return
    end

    -- join succ after timeout
    local player_map = wild.player_map_[_server_id]
    if not player_map then
        c_err( "[wild_mng](on_player_join_succ) player map not found, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )
        return
    end

    if player_map[_player_id] then
        c_err( "[wild_mng](on_player_join_succ) join timeout but still in wild, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
            _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )
        return
    end

    -- add player back to player_map
    player_map[_player_id] = true
    wild.player_num_ = wild.player_num_ + 1
    c_log( "[wild_mng](on_player_join_succ) join wild after timeout, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, player_num: %d",
        _wild_type, _zone_id, _wild_id, _server_id, _player_id, wild.player_num_ )
end

function remove_timeout_player( _joining_sn )
    local joining_info = g_joining_map[_joining_sn]
    g_joining_map[_joining_sn] = nil
    if not joining_info then
        c_err( "[wild_mng](remove_timeout_player) joining info not found, sn: %d", _joining_sn )
        return
    end

    local wild_type = joining_info.wild_type_
    local zone_id = joining_info.zone_id_
    local wild_id = joining_info.wild_id_
    local server_id = joining_info.server_id_
    local player_id = joining_info.player_id_

    local wild = get_wild( wild_type, zone_id, wild_id )
    if not wild then
        c_err( "[wild_mng](remove_timeout_player) wild not found, wild_type: %d, zone_id, %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d",
            wild_type, zone_id, wild_id, server_id, player_id, _joining_sn )
        return
    end

    -- clear joining sn from wild's joining sn map
    local sn_map = wild.joining_sn_map_[server_id]
    if not sn_map then
        c_err( "[wild_mng](remove_timeout_player) joining_sn map not found, wild_type: %d, zone_id, %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d",
            wild_type, zone_id, wild_id, server_id, player_id, _joining_sn )
        return
    end

    if not sn_map[player_id] then
        c_err( "[wild_mng](remove_timeout_player) joining_sn not found, wild_type: %d, zone_id, %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d",
            wild_type, zone_id, wild_id, server_id, player_id, _joining_sn )
        return
    end

    sn_map[player_id] = nil

    -- clear player from wild's player map
    local player_map = wild.player_map_[server_id]
    if not player_map then
        c_err( "[wild_mng](remove_timeout_player) player map not found, wild_type: %d, zone_id, %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d",
            wild_type, zone_id, wild_id, server_id, player_id, _joining_sn )
        return
    end

    if not player_map[player_id] then
        c_err( "[wild_mng](remove_timeout_player) not in player map, wild_type: %d, zone_id, %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d",
            wild_type, zone_id, wild_id, server_id, player_id, _joining_sn )
    end

    wild.player_num_ = wild.player_num_ - 1
    player_map[player_id] = nil

    c_log( "[wild_mng](remove_timeout_player) join timeout, wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d, joining_sn: %d, player_num: %d",
        wild_type, zone_id, wild_id, server_id, player_id, _joining_sn, wild.player_num_ )
end

function send_wild_list_to_client( _server_id, _dpid, _wild_type, _zone_id, _page_index )
    local zone_wilds = get_zone_wilds( _wild_type, _zone_id ) 
    local id_list = get_id_list( _wild_type, _zone_id )
    local wild_num = #id_list
    local page_num = ceil( wild_num / WILD_NUM_PER_PAGE )
    if page_num == 0 then
        page_num = 1
    end
    if _page_index > page_num then
        _page_index = page_num
    end

    local start_index = WILD_NUM_PER_PAGE * ( _page_index - 1 ) + 1
    local end_index = start_index + WILD_NUM_PER_PAGE - 1
    local result_list = {}
    for i = start_index, end_index, 1 do
        local wild_id = id_list[i]
        if wild_id then
            local wild = zone_wilds[wild_id]
            if not wild then
                c_err( "[wild_mng](send_wild_list_to_client) wild not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, wild_id )
                return
            end
            tinsert( result_list, { wild_id, wild.player_num_, wild.is_has_boss_ })
        end
    end
    local wild_cfg = WILDS[_wild_type]
    local max_player_num = wild_cfg.MaxPlayerNum

    bfsvr.remote_call_game( _server_id, "gamesvr.send_wild_list_to_client", _dpid, _wild_type, page_num, _page_index, max_player_num, result_list )
end

-- instance_t.on_add_obj
function on_instance_add_obj( _inst, _wild_type, _zone_id, _wild_id, _objid )
    local obj = ctrl_mng.get_ctrl( _objid )
    if (not obj) or (not obj:is_player()) then
        return
    end
    
    init_player(obj, _wild_type)

    bfsvr.remote_call_center( "wild_mng.on_player_join_succ", _wild_type, _zone_id, _wild_id, obj.server_id_, obj.player_id_ )
    local cnt = _inst:get_exorcist_cnt()
    if cnt > 0 then
        local other_text = MESSAGES[GUILDPROF_TASK_SYS_CFG.ExorcistAppear[1]]
        obj:add_defined_text( other_text )
    end

    egg_mng.on_player_join_wild( _inst,  obj )
end

-- instance_t.on_remove_obj
function on_instance_remove_obj( _inst, _wild_type, _zone_id, _wild_id, _objid )
end

-- player_t.before_join
function before_player_join( _player )
    local core = _player.core_
    local wild_camp = _player.wild_camp_
    if wild_camp then
        core:c_set_camp( wild_camp )
    end
    core:c_setpos( _player.wild_x_, 0, _player.wild_z_ )
    core:c_set_angle_y( _player.wild_angle_y_ )
end

local function do_enter_effects( _wild_type, _player )
    local wild_cfg = WILDS[_wild_type]
    local enter_effects = wild_cfg.EnterEffects
    if not enter_effects then
        return
    end
    for _, effect_def in ipairs( enter_effects ) do
        local func_name = effect_def[1]
        local func = rawget( wild_mng, func_name )
        if not func then
            c_err( "[wild_mng](do_enter_effects) effect func not found, wild_type: %d, effect name: %s", _wild_type, func_name )
            return 
        end
        if not func( wild_cfg, _player, effect_def ) then
            c_err( "[wild_mng](do_enter_effects) effect error, wild_type: %d, effect name: %s", _wild_type, func_name )
            return
        end
    end
end

-- PT_AFTER_ADD_HERO
function after_client_add_active_player( _player )
    local ok , plane = _player:is_in_wild()
    if not ok then 
        c_err( "[wild_mng](after_client_add_active_player) player not in wild, server_id: %d, player_id: %d", _player.server_id_, _player.player_id_ )
        bfsvr.send_player_back_to_game( _player )
        return 
    end 
        
    local x, _, z = _player.core_:c_getpos()
    local angle = _player.core_:c_get_angle_y()
    instance_mng.do_join_existed_instance( _player, plane.world_id_, plane.plane_id_, x, z, angle )

    do_enter_effects( plane.wild_type_, _player )

    plane:plane_check_and_delete_leak_object()
end

function before_player_leave( _player, _camp )
    -- 暂时直接调用副本的remove_obj，逻辑上应该有漏洞
    local instance = instance_mng.get_obj_instance( _player )
    if instance then
        instance:remove_obj( _player.ctrl_id_ )
    end
    -- 下线退出铭文状态，清理一下数据
    if _player:is_writing_inscription( ) then
        _player:end_writing_inscription_by_server( )
    end

    local plane = planemng.get_player_plane( _player )
    local wild_type = plane.wild_type_
    local zone_id = plane.zone_id_
    local wild_id = plane.wild_id_
    local camp = _camp
    local server_id = _player.server_id_
    local player_id = _player.player_id_

    bfsvr.remote_call_center( "wild_mng.leave_wild", wild_type, zone_id, wild_id, camp, server_id, player_id )

    c_log( "[wild_mng](before_player_leave) wild_type: %d, zone_id: %d, wild_id: %d, server_id: %d, player_id: %d",
        wild_type, zone_id, wild_id, server_id, player_id )
end

local function check_exit_cond( _wild_type, _player )
    local wild_cfg = WILDS[_wild_type]
    local cond_effects = wild_cfg.ExitConds
    if not cond_effects then
        return true
    end
    for _, effect_def in ipairs( cond_effects ) do
        local func_name = effect_def[1]
        local func = rawget( wild_mng, func_name )
        if not func then
            c_err( "[wild_mng](check_exit_cond) effect func not found, wild_type: %d, effect name: %s", _wild_type, func_name )
            return false
        end
        if not func( wild_cfg, _player, effect_def ) then
            return false
        end
    end
    return true
end

-- PT_QUIT_BATTLEFIELD
function on_player_quit_battlefield( _plane, _player, _is_active )
    if not _is_active then
        return
    end
    local wild_type = _plane.wild_type_
    local wild_cfg = WILDS[wild_type]
    local default_exit = wild_cfg.DefaultExit
    if not default_exit then
        return
    end
    local trans_cfg = WILDS_TRANSFER[default_exit]
    if not trans_cfg then
        return
    end
    if not check_exit_cond( wild_type, _player ) then
        return
    end
    local pos_list = trans_cfg.DstPos
    if pos_list then
        local pos_index = mrandom( #pos_list )
        local pos = pos_list[pos_index]
        local center_x = pos[1]
        local center_z = pos[2]
        local radius = mrandom( pos[3] )
        local radians = mrad( mrandom( 360 ) )
        local sin = msin( radians )
        local cos = mcos( radians )
        local x = radius * cos + center_x
        local z = radius * sin + center_z
        local hit_ret, hit_x, hit_z, hit_rate = c_raycast( channel_mng.CITY_SCENE_ID, 0, center_x, center_z, x, z )
        _player.last_city_x_ = hit_x
        _player.last_city_z_ = hit_z
        _player.x_ = hit_x
        _player.z_ = hit_z
    end
    bfsvr.send_player_back_to_game( _player )
end

on_add_player_func_map_ = 
{
    [3] = wild_mng.on_add_player_in_guild_inst,
    [4] = wild_mng.on_add_player_in_guild_inst,
}

on_dead_func_map_ =
{
    [1] = player_t.on_dead_in_wild,
    [2] = player_t.on_dead_in_wild,
    [3] = wild_mng.on_dead_in_guild_inst,
    [4] = wild_mng.on_dead_in_guild_inst,
    [5] = player_t.on_dead_in_wild,
    [6] = player_t.on_dead_in_wild,
    [7] = player_t.on_dead_in_wild,
    [8] = player_t.on_dead_in_wild,
    [9] = player_t.on_dead_in_wild,
    [10] = player_t.on_dead_in_wild,
    [11] = player_t.on_dead_in_wild,
    [12] = player_t.on_dead_in_wild,
}

on_damage_func_map_ =
{
    [1] = player_t.on_damage_in_wild,
    [2] = player_t.on_damage_in_wild,
    [5] = player_t.on_damage_in_wild,
    [6] = player_t.on_damage_in_wild,
    [7] = player_t.on_damage_in_wild,
    [8] = player_t.on_damage_in_wild,
    [9] = player_t.on_damage_in_wild,
    [10] = player_t.on_damage_in_wild,
    [11] = player_t.on_damage_in_wild,
    [12] = player_t.on_damage_in_wild,
}

on_before_leave_func_map_ =
{
    [3] = wild_mng.on_player_before_leave_in_guild_inst,
    [4] = wild_mng.on_player_before_leave_in_guild_inst,
}

function create_new_on_dead( _old_on_dead, _wild_type )
    return function ( _player, _A )
        _old_on_dead( _player, _A )
        --player_t.on_dead_in_wild( _player, _A )
        local plane = planemng.get_player_plane( _player )
        local wild_type = plane.wild_type_
        local wild_cfg = WILDS[wild_type]
        if wild_cfg.ReviveType == 1 then
            local revive_cd = wild_cfg.ReviveCD
            g_timermng:c_add_timer( revive_cd* frame_rate, timers.GUILD_INST_PLAYER_REVIVE, _player.ctrl_id_, 0, 0 )

        end
        local func = on_dead_func_map_[_wild_type]
        if not func then return end
        func( _player, _A )
    end
end

function create_new_on_damage( _old_on_damage, _wild_type )
    return function ( _player, _A, _dmg, _skill, _damage_type, _attack_type )
        _old_on_damage( _player, _A, _dmg, _skill, _damage_type, _attack_type )
        local func = on_damage_func_map_[_wild_type]
        if not func then return end
        func( _player, _A )
    end
end

function create_new_before_leave( _old_before_leave, _wild_type)
    return function ( _player )
        local camp = _player.core_:c_get_camp()
        _old_before_leave( _player )
        before_player_leave( _player, camp )
        local func = on_before_leave_func_map_[_wild_type]
        if not func then return end
        func( _player )
    end
end

function init_player( _player, _wild_type )
    local wild_cfg = WILDS[_wild_type]    

    -- player_t.on_dead
    local old_on_dead = _player.on_dead
    _player.on_dead = create_new_on_dead( old_on_dead, _wild_type )
    _player.old_on_dead_ = old_on_dead

    -- player_t.on_damage
    local old_on_damage = _player.on_damage
    _player.on_damage = create_new_on_damage( old_on_damage, _wild_type )
    _player.old_on_damage_ = old_on_damage

    -- player_t.before_leave
    local old_before_leave = _player.before_leave
    _player.before_leave = create_new_before_leave( old_before_leave, _wild_type )
    _player.old_before_leave_ = old_before_leave

    local inst = _player:get_instance()
    _player:restore_full_hp()
    
    if on_add_player_func_map_[_wild_type] then
        on_add_player_func_map_[_wild_type](_player, inst)
    end
end

function on_player_revive( _ctrl_id )
    local player = ctrl_mng.get_ctrl( _ctrl_id )
    if (not player) or (not player:is_player() ) then
        return
    end
    
    player:revive_in_wild()
end

function restore_player( _player )
    _player.on_dead = _player.old_on_dead_
    _player.on_damage = _player.old_on_damage_
    _player.before_leave = _player.old_before_leave_
end

-- 1.任意BF上远程调用该区野外函数
function bf_remote_call_zone_wild_do( _zone_id, _wild_type, _call_func, ... )
    if bfsvr.is_center() then 
        center_bf_handle_wild_remote_all( _zone_id, _wild_id, _call_func, ... )
    else
        bfsvr.remote_call_center( "wild_mng.center_bf_handle_wild_remote_all", _zone_id, _wild_type, _call_func, ... )
    end 
end 

-- 2.中心服处理该区野外请求
function center_bf_handle_wild_remote_all( _zone_id, _wild_type, _call_func, ... )
    if not bfsvr.is_center() then 
        c_err( "%s(center_bf_handle_wild_remote_all) not bf server", LOG_HEAD )
        return
    end 

    local zone_wilds = get_zone_wilds( _wild_type, _zone_id )
    for wild_id, wild in pairs ( zone_wilds ) do 
        local bf_id = get_wild_bf_id( wild_id )
        local inst_index = wild.inst_index_
        bfsvr.remote_call_bf( bf_id, "wild_mng.bf_wild_handle_center_remote_call", inst_index, _call_func, ... )
    end 
end 

-- 3.BF处理野外请求
function bf_wild_handle_center_remote_call( _inst_index, _call_func, ... )
    local inst = instance_mng.get_instance( _inst_index )
    if not inst then
        c_log( "[wild_mng](bf_wild_handle_center_remote_call) instance not found, inst_index: %d", _inst_index )
        return
    end

    local func = loadstring( sformat( "return _G.%s", _call_func ) )()
    if not func then
        c_err( "[wild_mng](bf_wild_handle_center_remote_call) failed to load function '%s'", _call_func )
        return
    end

    func( inst, ... )
end 

-- 同步中心服BOSS出现
function tell_center_my_wild_has_boss( _wild_type, _zone_id, _wild_id )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](tell_center_my_wild_has_boss) wild not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
        return
    end
    wild.is_has_boss_ = 1
    c_log( "[wild_mng](tell_center_my_wild_has_boss) has boss wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
end 

-- 同步中心服BOSS死亡
function tell_center_my_wild_boss_was_clear( _wild_type, _zone_id, _wild_id )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](tell_center_my_wild_boss_was_clear) wild not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
        return
    end
    wild.is_has_boss_ = 0
    c_log( "[wild_mng](tell_center_my_wild_boss_was_clear) boss kill wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
end 

-- 野外BOSS数量增加
function add_wild_boss_num( _plane, _num )
    if _num <= 0 then 
        c_err( "%s(add_wild_boss_num) num: %d", LOG_HEAD, _num )
        return 
    end 
    
    local wild_type = _plane.wild_type_
    local zone_id = _plane.zone_id_
    local wild_id = _plane.wild_id_

    local old_boss_num = _plane.boss_num_ 
    local new_boss_num = old_boss_num + _num 
    _plane.boss_num_ = new_boss_num 

    if old_boss_num == 0 then 
        bfsvr.remote_call_center( "wild_mng.tell_center_my_wild_has_boss", wild_type, zone_id, wild_id ) 
    end 

    c_log( "%s(add_wild_boss_num) zone_id: %d, wild_id: %d, old boss num: %d, new boss num: %d", LOG_HEAD, zone_id, wild_id, old_boss_num, new_boss_num) 
end 

-- 野外BOSS数量减少
function del_wild_boss_num( _plane, _num )
    if _num <= 0 then 
        c_err( "%s(del_wild_boss_num) num: %d", LOG_HEAD, _num )
        return 
    end 

    local wild_type = _plane.wild_type_
    local zone_id = _plane.zone_id_
    local wild_id = _plane.wild_id_

    local old_boss_num = _plane.boss_num_ 
    local new_boss_num = old_boss_num - _num 
    if new_boss_num < 0 then 
        new_boss_num = 0
    end 
    
    _plane.boss_num_ = new_boss_num 

    if new_boss_num == 0 then 
        bfsvr.remote_call_center( "wild_mng.tell_center_my_wild_boss_was_clear", wild_type, zone_id, wild_id ) 
    end 

    c_log( "%s(del_wild_boss_num) zone_id: %d, wild_id: %d, old boss num: %d, new boss num: %d", LOG_HEAD, zone_id, wild_id, old_boss_num, new_boss_num) 
end 

-------------------------------------------------------------------------------
--                           玩家离开场景检查条件
-------------------------------------------------------------------------------
function JUSTICE_GE( _wild_cfg, _player, _effect_def )
    local min_justice = _effect_def[2]
    local dialog_id = _effect_def[3]
    if _player.justice_ >= min_justice then
        _player.is_evil_ = 0
        return true
    else
        _player:add_show_scenario_dialog( dialog_id, 0, 0 )
        return false
    end
end

-------------------------------------------------------------------------------
--                           玩家进入场景触发效果
-------------------------------------------------------------------------------
function SHOW_DIALOG( _wild_cfg, _player, _effect_def )
    local dialog_id = _effect_def[2]
    _player:add_show_scenario_dialog( dialog_id, 0, 0 )
    return true
end



function report_player_info_to_centerbf()
    for unique_id, inst_index in pairs( g_unique_map ) do
        c_log( "[wild_mng](report_player_info_to_centerbf) unique_id: %d, inst_index: %d", unique_id, inst_index or 0 )
        local inst = instance_mng.get_instance( inst_index )
        if inst then
            local plane = inst.plane_ 
            local wild_type = plane.wild_type_
            local zone_id = plane.zone_id_
            local wild_id = plane.wild_id_

            c_log( "[wild_mng](report_player_info_to_centerbf) wild_type: %d, zone_id: %d, wild_id: %d", wild_type, zone_id, wild_id )

            local player_info_list = {}
            for ctrl_id, _ in pairs( plane.objects_ ) do
                local obj = ctrl_mng.get_ctrl( ctrl_id )
                if obj and obj:is_player() then
                    local server_id = obj.server_id_
                    local player_id = obj.player_id_
                    local camp = obj.core_:c_get_camp()
                    tinsert( player_info_list, { server_id, player_id, camp } )
                    c_log( "[wild_mng](report_player_info_to_centerbf) server_id: %d, player_id: %d, camp: %d", server_id, player_id, camp )
                end
            end
            bfsvr.remote_call_center( "wild_mng.reset_player_info", wild_type, zone_id, wild_id, player_info_list )
        end
    end
end

function reset_player_info( _wild_type, _zone_id, _wild_id, _player_info_list )
    local wild = get_wild( _wild_type, _zone_id, _wild_id )
    if not wild then
        c_err( "[wild_mng](reset_player_info) wild not found, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )
        return
    end

    c_log( "[wild_mng](reset_player_info) START, wild_type: %d, zone_id: %d, wild_id: %d", _wild_type, _zone_id, _wild_id )

    local all_player_map = {}
    local player_num = 0
    local camp16_num = 0
    local camp17_num = 0

    for _, player_info in ipairs( _player_info_list ) do
        local server_id = player_info[1]
        local player_id = player_info[2]
        local camp = player_info[3]
        c_log( "[wild_mng](reset_player_info) server_id: %d, player_id: %d, camp: %d", server_id, player_id, camp )

        local player_map = all_player_map[server_id]
        if not player_map then
            player_map = {}
            all_player_map[server_id] = player_map
        end
        player_map[player_id] = true
        player_num = player_num + 1
        if camp == GUILD_FIGHT_LEFT_CAMP then
            camp16_num = camp16_num + 1
        elseif camp == GUILD_FIGHT_RIGHT_CAMP then
            camp17_num = camp17_num + 1
        end
    end

    wild.player_map_ = all_player_map
    wild.player_num_ = player_num
    wild.camp_pcnt_[16] = camp16_num
    wild.camp_pcnt_[17] = camp17_num

    c_log( "[wild_mng](reset_player_info) DONE, wild_type: %d, zone_id: %d, wild_id: %d, player_num: %d, camp16: %d, camp17: %d",
        _wild_type, _zone_id, _wild_id, player_num, camp16_num, camp17_num )
end

g_is_player_info_reported = g_is_player_info_reported or false
if self_bf_id ~= 0 then
    if not g_is_player_info_reported then
        g_is_player_info_reported = true
        report_player_info_to_centerbf()
    end
end


