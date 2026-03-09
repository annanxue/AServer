module( "robot_team", package.seeall )

local g_ar = g_ar or LAr:c_new()


function send_invite_msg( _dpid, _server_id, _player_id ) 
    local ar = g_ar 
    ar:flush_before_send( msg.PT_TEAM_INVITE_MSG ) 
    ar:write_int( _server_id ) 
    ar:write_int( _player_id ) 
    ar:send_one_ar( g_robotclient, _dpid ) 
end

function send_client_action( _dpid, _action, _server_id, _player_id ) 
    local ar = g_ar 
    ar:flush_before_send( msg.PT_TEAM_CLIENT_ACTION ) 
    ar:write_ubyte( _action ) 
    ar:write_int( _server_id ) 
    ar:write_int( _player_id ) 
    ar:send_one_ar( g_robotclient, _dpid ) 
end

-- 发送确认消息
function send_confirm_msg( _dpid, _confirm_action, _result ) 
    local ar = g_ar 
    ar:flush_before_send( msg.PT_TEAM_CONFIRM_MSG ) 
    ar:write_short( _confirm_action ) 
    ar:write_ubyte( _result ) 
    ar:send_one_ar( g_robotclient, _dpid ) 
end

function send_trasleader( _dpid, _server_id, _player_id ) 
   send_client_action( _dpid,  team_t.TEAM_ACTION_TRANSFER_LEADER, _server_id, _player_id )
end

function send_drop_team( _dpid ) 
    send_client_action( _dpid, team_t.TEAM_ACTION_QUIT_TEAM ) 
end


function on_team_invite_msg( _ar, _dpid,  _size ) 
    local server_id = _ar:read_int() 
    local player_id = _ar:read_int() 
    local name = _ar:read_string() 
    local action = _ar:read_ubyte() 
    send_client_action( _dpid, team_t.TEAM_ACTION_ACCEPT_INVITE, server_id, player_id )

    --local hero = robotclient.get_hero( _dpid ) 
    --local rate = math.random( 0, 10000 )
    --if rate < 6000  then 
    --    send_client_action( _dpid, team_t.TEAM_ACTION_ACCEPT_INVITE, server_id, player_id )
    --    c_log( "player:%s accept sid:%d pid;%d", hero.player_name_, server_id, player_id  )
    --elseif rate < 8000 then 
    --    send_client_action( _dpid, team_t.TEAM_ACTION_REFUSE_INVITE, server_id, player_id )
    --    c_log( "player:%s refuse sid:%d pid;%d", hero.player_name_, server_id, player_id  )
    --end
    --c_log( "player;%s id:%d dpid:%x, serverid:%d, playerid:%d, name:%s action:%x", hero.player_name_, _dpid, 
    --        hero.player_id_,  server_id, player_id, name, action )  
end

function on_got_team_data(_ar,  _dpid, _size ) 
    local t = team_t.deserialize( _ar ) 
    local hero = robotclient.get_hero( _dpid ) 
    hero.team_data_ = hero.team_data_ or {}
    hero.team_data_.team_ = t
    team_t.init( t ) 
    c_log( "[on_got_team_data]hero:%s, playerid:%d", hero.player_name_, hero.player_id_ )
end

function on_team_server_action(  _ar, _dpid,_size ) 
    local msg = _ar:read_short()
    local hero = robotclient.get_hero( _dpid ) 
    if msg == team_t.TEAM_SERVER_MSG_PLAYER_KIECKED then 
        local team_data = hero.team_data_
        if team_data then 
            team_data.team_ = nil
        end
    elseif msg == team_t.TEAM_SERVER_MSG_PLAYER_QUIT_TEAM then 
        local team_data = hero.team_data_
        if team_data then 
            team_data.team_ = nil
        end
    else

    end
    c_log( "[on_team_server_action]hero:%s, playerid:%dk msg:%d", hero.player_name_, hero.player_id_, msg )
end

function on_confirm_action( _ar, _dpid, _size ) 
    local msg = _ar:read_short() 
    send_confirm_msg( _dpid, msg, 0x01 )    
end

function member_changed_attribute( _ar, _dpid, _size ) 
    local server_id = _ar:read_int() 
    local player_id = _ar:read_int() 
    local listen_action = _ar:read_ubyte() 
    local value = _ar:read_int() 
    local  hero = robotclient.get_hero( _dpid ) 
    c_log( " member sid:%d pid:%d la:%d value:%d",  server_id, player_id, listen_action, value )
end

function on_team_match_fail( _ar, _dpid, _size ) 
    local result = _ar:read_int()
    local server_id = _ar:read_int() 
    local player_id = _ar:read_int() 
    c_log( "sid:%d pid:%d result:%d  match fail", server_id, player_id, result  )
end

function on_team_set_state( _ar, _dpid, _size ) 
    local team_state = _ar:read_ubyte() 
    local  hero = robotclient.get_hero( _dpid ) 
    c_log( "team_state:%d hero:%s", team_state,  hero.tostr_ ) 
end

function on_team_match_state( _ar, _dpid, _size ) 
    local match_state = _ar:read_ubyte() 
    local  hero = robotclient.get_hero( _dpid ) 
    c_log( "on_team_match_state:%d hero:%s",  match_state, hero.tostr_ )
end



function after_hero_added( _dpid, _hero ) 
    _hero.team_data_ = { 
    } 
    local t = (_hero.player_id_ * 100) % 30 
    g_timermng:c_add_timer_second( t, timers.UPDATE_TEAM_TIMER, _dpid, 0, 0 ) 
end


function set_random_timer( _dpid ) 
    math.randomseed( os.time() )
    local time = math.random(3, 30 )
    g_timermng:c_add_timer( time, timers.UPDATE_TEAM_TIMER, _dpid, 0, 0 ) 
end

function send_invite( _hero ) 
    local dpid = _hero.dpid_ 
    local list = ctrl_mng.g_ctrl_mng[ dpid ] 
    local tcount = math.random( 3, 100 )
    local   c = 0 
    for ctrl_id, obj in pairs( list ) do 
        if obj:is_player() then 
            local rate = math.random( 100 )
            if rate > 50 then 
                c = c+ 1
                send_invite_msg( dpid,  obj.server_id_, obj.player_id_ )
                if c >= tcount then return end 
            end
        end
    end
    c_log( "[send_invite] hero:%s", _hero.player_name_ )
end


function on_timer( _dpid ) 
    local hero = robotclient.get_hero( _dpid ) 
    if not hero then  --  玩家没了
        return 
    end
    
    set_random_timer( _dpid )

    if  not hero.team_data_ then 
        hero.team_data_   = {}
    end

    local team = hero.team_data_.team_
    if not team then 
       -- 没有队伍  随机向周围的人的发送邀请队伍 
        send_invite( hero ) 
    else
        local member_count = #team.member_list_ 
        if member_count < 3 then 
            local rate = math.random( 0, 100 )
            if rate < 10 then 
                send_client_action( _dpid, team_t.TEAM_ACTION_QUIT_TEAM )  
                c_log( "[%s] I going to quit", hero.player_name_ )
            else
                send_invite(  hero ) 
            end 
        else

            if team_t.is_leader(team,  hero.server_id_, hero.player_id_ )  then 
                local rate = math.random( 0, 100 )
                if rate < 40 then 
                    for _, m in pairs( team.member_list_ ) do 
                        if m.server_id_ ~= hero.server_id_ or m.player_id_ ~= hero.player_id_ then 
                            send_client_action( _dpid, team_t.TEAM_ACTION_TRANSFER_LEADER, m.server_id_, m.player_id_ )
                            c_log( "player:%s transfer leader to :%s", hero.player_name_, m.player_name_ )
                            return
                        end
                    end
                elseif rate < 70  then 
                    for _, m in pairs( team.member_list_ ) do 
                        if m.server_id_ ~= hero.server_id_ or m.player_id_ ~= hero.player_id_ then 
                            send_client_action( _dpid, team_t.TEAM_ACTION_KICK_PLAYER, m.server_id_, m.player_id_ )
                            c_log( "player:%s kick :%s", hero.player_name_, m.player_name_ )
                            return
                        end
                    end
                end
            end

           math.randomseed( os.time() )
           local rate = math.random( 0, 100 )
           if rate < 30 then 
                send_client_action( _dpid, team_t.TEAM_ACTION_QUIT_TEAM )  
                c_log( "[%s] I going to quit", hero.player_name_ )
            elseif rate < 90 then 
                c_log( "[%s] I have nothing to do", hero.player_name_ )
            end
        end
    end
end


function test_send_invite( ) 
   for dpid, list in pairs( ctrl_mng.g_ctrl_mng ) do 
        for _, ctrl in pairs( list ) do
            if ctrl:is_player() then 
                send_invite_msg( dpid, ctrl.server_id_, ctrl.player_id_ ) 
            end
        end
    end
end

function test_drop_team() 
    for dpid, list in pairs( ctrl_mng.g_ctrl_mng ) do 
        send_client_action( dpid, 0x03, 0, 0 )
    end
end

function join_t( _dpid ) 
    local ar = g_ar 
    ar:flush_before_send( msg.PT_JOIN_BATTLEFIELD_MATCH ) 
    ar:write_int( 2 ) 
    ar:send_one_ar( g_robotclient, _dpid ) 
end

function quit_t( _dpid ) 
    local ar = g_ar 
    ar:flush_before_send( msg.PT_QUIT_BATTLEFIELD_MATCH ) 
    ar:write_int( 2 ) 
    ar:send_one_ar( g_robotclient, _dpid ) 
end

function send_join() 
   for dpid, list in pairs( ctrl_mng.g_ctrl_mng ) do 
        join_t( dpid  )  
    end
end

function send_quit() 
    for dpid, list in pairs( ctrl_mng.g_ctrl_mng ) do 
        quit_t( dpid  )  
    end
end

function send_invite_team( _obj, _obj2 ) 
    local sid = _obj2.server_id_
    local pid = _obj2.player_id_ 
    
    local ar = g_ar 
    ar:flush_before_send( msg.PT_TEAM_INVITE_MSG ) 
    ar:write_int( sid ) 
    ar:write_int( pid ) 
    ar:send_one_ar( g_robotclient, _obj.dpid_ ) 
    c_log( "player:%s invite player:%s", _obj.player_name_, _obj2.player_name_ )
end



