module( "robot_tamriel_match", package.seeall )

local table_len = table.getn
local t_insert = table.insert
local get_ctrl = ctrl_mng.get_ctrl
g_ar = g_ar or LAr:c_new()

g_index_player_map = g_index_player_map or {}

g_leader_index_map = g_leader_index_map or {}

function after_hero_added( _dpid,  _player ) 
    local dpid = _dpid 
    local ctrl_id = _player.ctrl_id_ 
    t_insert( g_index_player_map,{ dpid_ = dpid, ctrl_id_ = ctrl_id } ) 
    local time = math.random( 1 , 30 )
    g_timermng:c_add_timer_second( t, timers.ROBOT_TAMERIEL_MATCH, _dpid, 0, time ) 
end
--[[
function send_join_match( _dpid ) 
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    if dpid_info.tamriel_sign_  then 
        return 
    end 

    local ar = g_ar
    ar:flush_before_send( msg.PT_JOIN_BATTLEFIELD_MATCH ) 
    ar:write_byte( 1 ) 
    ar:write_byte( 1 )
    ar:send_one_ar( g_robotclient, _dpid )
end


function on_tamriel_sign_result( _ar, _dpid, _size ) 
    local result =  _ar.read_ubyte()
    local param  =  _ar.read_ubyte() 
    if result == 0 then  -- SIGN_SUCC
        local dpid_info = robotclient.g_dpid_mng[_dpid]
        dpid_info.tamriel_sign_ = true
    end 
end 

]]
--------------------------------
--local  function
---------------------------------
--[[
local function send_quit_match( _dpid ) 
    local ar = g_ar
    ar:flush_before_send( msg.PT_QUIT_BATTLEFIELD_MATCH ) 
    ar:write_int( 2 ) 
    ar:send_one_ar( g_robotclient, _dpid )
end

function show_player() 
    local count = #g_index_player_map
    for i = count, 1, -1 do 
        local item  = g_index_player_map[ i ]
        local dpid = item.dpid_
        if not ctrl_mng.g_ctrl_mng[ dpid ] then 
            table.remove( g_index_player_map, i )
        end
    end
    for index, item in pairs( g_index_player_map ) do 
        local obj = get_ctrl( item.dpid_, item.ctrl_id_ ) 
        if obj then 
            c_log( "[show player: index:%d  dpid:0x%x obj:%s", index, item.dpid_, obj.player_name_  ) 
        end
    end
end

function show_team() 
    g_leader_index_map = {}
    for index, item in pairs( g_index_player_map ) do 
        local obj = get_ctrl( item.dpid_, item.ctrl_id_ ) 
        if obj then 
            if obj.team_data_ and obj.team_data_.team_ then 
                local team = obj.team_data_.team_
                if team.leader_player_id_ == obj.player_id_ then 
                    t_insert( g_leader_index_map, { dpid_ = item.dpid_, ctrl_id_ = item.ctrl_id_ } )
                end
            end
        end
    end

    for index, item in pairs( g_leader_index_map ) do 
        local obj = get_ctrl( item.dpid_, item.ctrl_id_ ) 
        if obj and obj.team_data_ then 
            local team = obj.team_data_.team_
            local str = ""
            for _, m in pairs( team.member_list_ ) do 
                str = string.format("%sname:%s  ", str,  m.player_name_)
            end
            c_log( "index:%d team:%s", index, str )
        end
    end
end


function invite( _from_index, _to_index ) 
    local item = g_index_player_map[ _from_index ] 
    local item2 = g_index_player_map[ _to_index ]
    if not item or not item2 then 
        c_err( "[invite] index is wrong" ) 
        show_player()
        return
    end

    local obj1 = get_ctrl( item.dpid_, item.ctrl_id_ ) 
    local obj2 = get_ctrl( item2.dpid_, item2.ctrl_id_ ) 
    if not obj1 or not obj2 then 
        c_err( "[invite] obj is wrong" ) 
        show_player()
        return
    end

    robot_team.send_invite_team( obj1, obj2 )  
end

function team_join_match( ... ) 
    local args = {...}
    for _, index in pairs( args ) do 
        local item = g_leader_index_map[ index ] 
        if item then 
            local obj = get_ctrl( item.dpid_, item.ctrl_id_ ) 
            if obj then 
                send_join_match( item.dpid_ )  
            end
        else
            c_err( "index:%s is error" )
        end
    end
end

function team_quit_match( ... ) 
    local args = {...}
    for _, index in pairs( args ) do 
        local item = g_leader_index_map[ index ] 
        if item then 
            local obj = get_ctrl( item.dpid_, item.ctrl_id_ ) 
            if obj then 
                send_quit_match( item.dpid_ )  
            end
        else
            c_err( "index:%s is error" )
        end
    end

end



function join_match( ... ) 
    local args = { ... } 
    for _, index in pairs( args ) do 
        local item = g_index_player_map[ index ] 
        if not item then 
            c_err( "no such index" )
        else
            send_join_match( item.dpid_ ) 
        end
    end
end


function quit_match( ... ) 
    local args = { ... } 
    for _, index in pairs( args ) do 
        local item = g_index_player_map[ index ] 
        if not item then 
            c_err( "no such index" )
        else
            send_quit_match( item.dpid_ ) 
        end
    end
end

]]




