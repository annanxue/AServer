module( "robot_gm", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber
local mfloor, mrandom, mmin, ostime, osdifftime = math.floor, math.random, math.min, os.time, os.difftime

function send_gm_msg( _dpid, _func, _args )
    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_GM_MSG )
    ar:write_string( _func )
    ar:write_string( _args )
    ar:send_one_ar( g_robotclient, _dpid )
end

function level( _hero, _level )
    local args = sformat( "%d %d", _level, _hero.player_id_ )
    send_gm_msg( _hero.dpid_, "level", args )
end

function finish_inst( _hero )
    local args = sformat( "%d", _hero.player_id_ )
    send_gm_msg( _hero.dpid_, "finish_inst", args )
end

function energy( _hero, _energy )
    local args = sformat( "%s %d %d", "energy_", _energy, _hero.player_id_ )
    send_gm_msg( _hero.dpid_, "sv", args )
end
