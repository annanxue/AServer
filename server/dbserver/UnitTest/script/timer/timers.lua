module( "timers", package.seeall )

g_func_map = g_func_map or {}

---------------------------------------------
--              timer id 
---------------------------------------------
CLEAR_OFFLINE_OPS = 1

---------------------------------------------
--              timer func
---------------------------------------------
g_func_map[CLEAR_OFFLINE_OPS] = function( _server_id, _player_id )
    db_player_offline_op.do_delete_expired_op( )
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
