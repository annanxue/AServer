module( "robot_move", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber
local mfloor, mrandom, mmin, ostime, osdifftime = math.floor, math.random, math.min, os.time, os.difftime

local NPC_POS =
{
    { 439, 443 },
    { 430, 444 },
    { 414, 440 },
    { 418, 457 },
    { 349, 437 },
    { 446, 501 },
    { 483, 463 },
    { 459, 513 },
    { 425, 443 },
    { 483, 472 },
    { 418, 457 },
    { 446, 501 },
    { 439, 425 },
    { 349, 437 },
    { 425, 443 },
    { 354, 428 },
    { 430, 444 },
    { 414, 440 },
    { 483, 472 },
    { 483, 463 },
    { 439, 443 },
    { 459, 513 },
}

function after_hero_added( _dpid, _hero )
    local ctrl_id = _hero.ctrl_id_
    local interval = g_timermng:c_get_frame_rate() * 5
    g_timermng:c_add_timer( interval, timers.RANDOM_MOVE_TO, _dpid, ctrl_id, interval ) 
    c_log( "[robot_move] after_hero_added, start random move to, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
end

function timer_send_random_move_to( _dpid, _ctrl_id )
    local hero = ctrl_mng.get_ctrl( _dpid, _ctrl_id )
    if not hero then return 0 end

    local core = hero.core_
    local nav_type = 3 -- states.TASK_NAV
    local nav_rate = 1
    local sx, _, sz = core:c_getpos()
    local angle_y = core:c_get_angle_y()
    local target_index = mrandom( #NPC_POS )
    local target_pos = NPC_POS[target_index]
    local find_ret, find_path = c_findpath( 1001, 1, sx, sz, target_pos[1], target_pos[2] )
    if not find_ret then
        return 1
    end

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_STATE_NAVIGATE )
    ar:write_compress_pos_angle( sz, sz, angle_y )
    ar:write_ubyte( #find_path )
    for _, pos in ipairs( find_path ) do
        ar:write_float( pos[1] )
        ar:write_float( pos[2] )
    end
    ar:write_float( nav_rate )
    ar:write_int( nav_type )
    ar:write_int( -1 )
    ar:send_one_ar( g_robotclient, _dpid )

    return 1

--[[
    local ctrl = ctrl_mng.get_ctrl( _dpid, _ctrl_id )
    if not ctrl then return 0 end

    local core = ctrl.core_
    local x, y, z = core:c_getpos()
    local new_x = mrandom( x - 20, x + 20 )
    local new_z = mrandom( z - 20, z + 20 )
    local new_angle = mrandom( 0, 359 )

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_CORR_POS )
    ar:write_compress_pos_angle( new_x, new_z, new_angle )
    ar:write_ubyte( 1 )  -- arive not stop
    ar:send_one_ar( g_robotclient, _dpid )

    return 1
--]]
end
