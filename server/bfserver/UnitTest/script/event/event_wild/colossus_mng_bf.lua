-- 符石巨像活动
module( "colossus_mng", package.seeall )

local tinsert = table.insert
local ostime = os.time
local LOG_HEAD = "[colossus_mng]"
local is_table_empty = utils.table.is_empty

local frame_rate = g_timermng:c_get_frame_rate()


PICK_POINT_REFRESH_CONFIG_ID_1 = 9
PICK_POINT_REFRESH_CONFIG_ID_2 = 15

function is_colossus( _cfg_id )
    local cfg = PICK_POINTS_REFRESH_CONFIG[_cfg_id]
    if not cfg then
        return false
    end
    return cfg.Extra == 1
end


function on_refresh( _mon, _cfg_id )
    -- [server_id][player_id] = total_damage
    _mon.colossus_data_ = {}
    _mon.colossus_refresh_id_ = _cfg_id
    local old_on_damage = _mon.on_damage
    _mon.on_damage = function ( self, _A, _dmg, _skill, _damage_type, _attack_type )
        old_on_damage( self, _A, _dmg, _skill, _damage_type, _attack_type )
        on_mon_damage( self, _A, _dmg )
    end
end

function on_stop_refresh()
    --TODO DoNothing?
end

function get_data_by_mon( _mon )
    return _mon.colossus_data_
end

function on_mon_damage( _mon, _player, _damage )

    local server_id = _player.server_id_
    local player_id = _player.player_id_


    local colossus_refresh_id = _mon.colossus_refresh_id_
    if is_player_rewarded( _player, colossus_refresh_id ) then
        c_trace("[colossus_mng](on_mon_damage) already rewarded, mon_ctrl:%d, server_id:%d, player_id:%d, damage:%d",
            _mon.ctrl_id_, server_id, player_id, _damage)
        return
    end

    
    local data = get_data_by_mon( _mon )
    if not data then
        c_err("[colossus_mng](on_mon_damage) nil data, mon_ctrl:%d", _mon.ctrl_id_ or -1)
        return
    end
    local pmap = data[server_id]
    if not pmap then
        pmap = {}
        data[server_id] = pmap
    end
    local pdata = pmap[player_id]
    if not pdata then
        pdata = {_damage, _player.player_name_}
        pmap[player_id] = pdata
        c_trace("[colossus_mng](on_mon_damage) new, mon_ctrl:%d, server_id:%d, player_id:%d, damage:%d",
            _mon.ctrl_id_, server_id, player_id, _damage)
        return
    end
    local total_damage = pdata[1] + _damage
    pdata[1] = total_damage 
    c_trace("[colossus_mng](on_mon_damage) add, mon_ctrl:%d, server_id:%d, player_id:%d, damage:%d, total_damage:%d",
        _mon.ctrl_id_, server_id, player_id, _damage, total_damage)
end

local function only_top_n( _list, _n, _server_id, _player_id, _damage )
    local len = #_list
    local index = 1
    if len > 0 then
        for i = len, 1, -1 do
            local info = _list[i]
            local info_damage = info[3]
            if _damage < info_damage then
                index = i + 1
                break
            end
        end
    end
    if index > _n then
        return
    end
    local new_info = {_server_id, _player_id, _damage }
    tinsert(_list, index, new_info )
    _list[_n+1] = nil
end

local function get_top_n( _data, _n )
    local top_list = {}
    for server_id, pmap in pairs( _data ) do
        for player_id, pdata in pairs( pmap ) do
            only_top_n( top_list, _n, server_id, player_id, pdata[1] )
        end
    end
    return top_list
end


local function merge_list2map( _list, _from, _to, _dst_map )
    for i = _from, _to do
        local info = _list[i]
        if info then
            local server_id = info[1]
            local player_id = info[2]
            local player_name = info[4]

            local pmap = _dst_map[server_id]
            if not pmap then
                pmap = {}
                _dst_map[server_id] = pmap
            end
            pmap[player_id] = 1
        end
    end
end

function rank_mon( _mon )
    local data = get_data_by_mon( _mon )
    if not data then
        c_err("[colossus_mng](rank_mon) nil data, mon_ctrl:%d", _mon.ctrl_id_ or -1)
        return
    end
    local colossus_refresh_id = _mon.colossus_refresh_id_
    local type_id = _mon.type_id_
    for server_id, pmap in pairs( data ) do
        start_reward( server_id, pmap, colossus_refresh_id, type_id ) 
    end
end


function start_reward(_server_id, _pmap, _colossus_refresh_id, _type_id )
    -- 人数多的时候分帧, 防止人数多的时候引起卡顿的问题
    add2divide(true, _server_id, _pmap, _colossus_refresh_id, _type_id )
end

function send_data_to_gs(_server_id, _pmap, _colossus_refresh_id, _type_id )
    bfsvr.remote_call_game( _server_id, "colossus_mng.on_recv_data", _server_id, _pmap, _colossus_refresh_id, _type_id)
end

local function add_one_test_player( i, mon, list )
    local server_id = 72
    local player_id = i
    local player = { 
        server_id_ = server_id, player_id_ = player_id, small_data_ = {},
        pick_item_list_safe = function(self) end,
        add_reward_item_list = function(self) end,
        add_time_event_on = function(self) end,
        get_colossus_map = function(self)
            return player_t.get_colossus_map(self)
        end,
        add_push_bulletin = function(self) end,
    }
    if not player_mng.g_player_mng[server_id] then
        player_mng.g_player_mng[server_id] = {}
    end
    player_mng.g_player_mng[server_id][player_id] = player
    local damage = math.random(1,1000000)
    player.damage_ = damage
    on_mon_damage( mon, player, damage )
    table.insert(list, player)
end


-- 测试divide能够去掉已处理的
function test_divide()
end



