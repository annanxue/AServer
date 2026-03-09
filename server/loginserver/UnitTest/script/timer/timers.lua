module( "timers", package.seeall )

local sformat, min = string.format, math.min

g_func_map = g_func_map or {}

---------------------------------------------
--              timer id 
---------------------------------------------

GAME_HEARTBEAT = 1
LOGIN_KEY_EXPIRED = 2
CHECK_GAMESVR_CFG = 3

---------------------------------------------
--              timer func
---------------------------------------------
g_func_map[GAME_HEARTBEAT] = function ( _p1, _p2 )
    return game_login_server.on_heartbeat_timeout(_p1, _p2) 
end

g_func_map[LOGIN_KEY_EXPIRED] = function( _p1, _p2 )
    player_login_server.on_login_key_expired( _p1, _p2 )
    return 0
end

g_func_map[CHECK_GAMESVR_CFG] = function( _p1, _p2 )
    player_login_server.check_gamesvr_cfg()
    return 1
end

---------------------------------------------
--              func_map
---------------------------------------------

function handle_timer( _id, _p1, _p2, _index )
    local rt = 0
    local func = g_func_map[_id]
    if func then
        rt = func( _p1, _p2 )
    else
        c_err( string.format( "Unknown timer id: %d", _id ) )
    end
    return rt 
end
