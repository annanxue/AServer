module( "gm_cmd", package.seeall )

local sformat = string.format

function query_player_info( _dpid, _browser_client_id, _player_id, _gamesvr_id )
    if not _browser_client_id then
        return false, err( "[gm_cmd](query_player_info) browser_client_id not specified, "..
                           "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end
    if not _player_id then
        return false, err( "[gm_cmd](query_player_info) player_id not specified, dpid: 0x%X", _dpid )
    end
    if not _gamesvr_id then
        return false, err( "[gm_cmd](query_player_info) gamesvr_id not specified, dpid: 0x%X", _dpid )
    end

    local player_id = tonumber( _player_id )
    local gamesvr_id = tonumber( _gamesvr_id )

    local player = player_mng.get_player( gamesvr_id, player_id )
    if not player then
        bfsvr.remote_call_game( gamesvr_id, "dbclient.gm_query_player_info", player_id )
        c_gm( "[gm_cmd](query_player_info) player is not on bf, "..
               "send query to db, player_id: %d, gamesvr_id: %d", player_id, gamesvr_id )
        return
    end

    local player_info_json = build_player_info_json( player )

    if _dpid == 0 then  -- 该函数由game远程调用
        gmclient.send_gm_cmd_result( _browser_client_id, true, player_info_json )
        c_gm( "[gm_cmd](query_player_info) player_id: %d, gamesvr_id: %d", player_id, gamesvr_id )
    else  -- 该函数由gmserver直接向bf调用
        return true, player_info_json
    end
end

function send_player_back_to_game( _dpid, _browser_client_id, _player_id, _gamesvr_id )
    local player_id = tonumber( _player_id )
    if not player_id then
        return false, err( "[gm_cmd](send_player_back_to_game) player_id is not specified or not valid" )
    end
    local gamesvr_id = tonumber( _gamesvr_id )
    if not gamesvr_id then
        return false, err( "[gm_cmd](send_player_back_to_game) gamesvr_id is not specified or not valid" )
    end
    local player = player_mng.get_player( gamesvr_id, player_id )
    if player then
        bfsvr.send_player_back_to_game( player )
        return true
    end
    return false, err( "[gm_cmd](send_player_back_to_game) player is not on this bf, server_id: %d, player_id: %d", gamesvr_id, player_id )
end

function friend_mng_clear( _dpid, _browser_client_id )
    friend_mng.clear_center()
    return true
end

function force_end_weekend_pvp( _dpid, _browser_client_id, _is_rank_result )
    if _is_rank_result == "true" then
        event_weekend_pvp.weekend_pvp_event_end( true )
        c_gm("[gm_cmd](force_end_weekend_pvp)is_rank_result: true")
        return true
    elseif _is_rank_result == "false" then
        event_weekend_pvp.weekend_pvp_event_end( false )
        c_gm("[gm_cmd](force_end_weekend_pvp)is_rank_result: false")
        return true
    else
        return false, "check _is_rank_result: true/false"
    end
end

function start_weekend_pvp( _dpid, _browser_client_id )
    event_weekend_pvp.on_weekend_pvp_enroll_start()
    return true
end

function start_guild_daily_pvp( _dpid, _browser_client_id )
    guild_daily_pvp.bf_start_enroll()
    return true
end

function force_end_guild_daily_pvp( _dpid, _browser_client_id )
    guild_daily_pvp.bf_finish_event()
    return true
end

function dump_dead_monster_memory( _dpid, _browser_client_id , _scene_id  )
  --  local player = player_mng.get_player_by_player_id( tonumber(_player_id) )
  --  if not player then
  --      return false  , "not found player "
   -- end
    
    --local instance =  instance_mng.get_obj_instance(player)
    --if not instance then
    --    return false , "not find instance"
    --end 

    local time = c_cpu_ms()
    --local plane = instance.plane_
    
    local ncount = 0 
    local plane_list = planemng.g_plane_mng[tonumber(_scene_id)]
    if not plane_list then 
        return false , "not find plane_list"
    end 

    for _ , plane in pairs(plane_list) do 
        for objid , _  in pairs(plane.objects_) do 
            local monster = ctrl_mng.get_ctrl(objid)
            if monster and monster:is_dead() then  -- 
                local monster_str = utils.vertical_serialize_table(monster)
                local file_name = string.format("./%d_dead.lua", objid)
                local f = io.open(file_name, "w+") 
                f:write( monster_str  )
                f:close()
    
                ncount = ncount + 1 
    
                if ncount > 5 then 
                    return true , "success1  cost " .. ( c_cpu_ms() - time ) 
                end 
            end 
        end     
    end 

    return true , "success2  cost " .. ( c_cpu_ms() - time ) 
end

