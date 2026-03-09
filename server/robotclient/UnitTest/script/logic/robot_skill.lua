module( "robot_skill", package.seeall )

local sformat, tinsert, tremove, unpack, msg, tonumber = string.format, table.insert, table.remove, unpack, msg, tonumber
local mfloor, mrandom, mmin, ostime, osdifftime = math.floor, math.random, math.min, os.time, os.difftime

local function random_skill_id( _player )
    local job_id = _player.job_id_
    if job_id == JOBS.JOB_ID_JOB1 then
        local skills = { 200501, 200502, 200503, 200504 }
        local index = mrandom( #skills )
        return skills[index]
    elseif job_id == JOBS.JOB_ID_JOB2 then
        local skills = { 202001, 202002, 202003, 202004 }
        local index = mrandom( #skills )
        return skills[index]
    elseif job_id == JOBS.JOB_ID_JOB3 then
        local skills = { 206001, 206002, 206003, 206004, 206005, 206006 }
        local index = mrandom( #skills )
        return skills[index]
    elseif job_id == JOBS.JOB_ID_JOB4 then
        local skills = { 204123, 20412301, 20412302, 20412303, 20412304, 20412305 }
        local index = mrandom( #skills )
        return skills[index]
    end
    return 0
end

function after_hero_added( _dpid, _hero )
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    local ctrl_id = _hero.ctrl_id_
    --if dpid_info.dpid_type_ == robotclient.DPID_TYPE_BF then
        g_timermng:c_add_timer_second( 2, timers.RANDOM_DO_SKILL, _dpid, ctrl_id, 5 )
        c_log( "[robot_skill] after_hero_added, start random do skill, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
    --end
end

function timer_send_random_do_skill( _dpid, _ctrl_id )
    local ctrl = ctrl_mng.get_ctrl( _dpid, _ctrl_id )
    if not ctrl then return 0 end

    local core = ctrl.core_
    local x, y, z = core:c_getpos()
    local angle = mrandom(360)
    local skill_id = random_skill_id( ctrl )
    local dpid_info = robotclient.g_dpid_mng[_dpid]
    local secret = dpid_info.skill_secret_
    if not secret then return 1 end
    c_log( "[robot_skill](timer_send_random_do_skill)dpid: 0x%X, ctrl_id: %d, skill_id: %d, secret: %d", _dpid, _ctrl_id, skill_id, secret )

    local ar = robotclient.g_ar
    ar:flush_before_send( msg.PT_STATE_SKILL )
    ar:write_float( c_cpu_ms()/1000.0 )
    ar:write_int( secret )
    ar:write_int( -1 )
    ar:write_int( c_xor(skill_id, secret) )
    ar:write_compress_pos_angle( x, z, angle )
    ar:send_one_ar( g_robotclient, _dpid )

    return 1
end
