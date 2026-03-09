module( "logserver", package.seeall )
local tinsert = table.insert
local ostime = os.time
local osdate = os.date

local frame_rate = g_timermng:c_get_frame_rate()
ONLINE_LOG_PERIOD = 60 * frame_rate
OVERDUE_PERIOD = 5

gs_online_map_ = gs_online_map_ or {}

if timer and timer >= 0 then
    g_timermng:c_del_timer( timer )
end

timer = g_timermng:c_add_timer( ONLINE_LOG_PERIOD, timers.LOGSERVER_PROC, 0, 0, ONLINE_LOG_PERIOD )

local function string2time(timeString)
    if type(timeString) ~= 'string' then 
        c_err('string2time: timeString is not a string') 
        return 0
    end

    local fun = string.gmatch( timeString, "%d+")
    local y = fun() or 0
    if y == 0 then c_err('timeString is a invalid time string: %s', timeString) return 0 end
    local m = fun() or 0
    if m == 0 then c_err('timeString is a invalid time string: %s', timeString) return 0 end
    local d = fun() or 0
    if d == 0 then c_err('timeString is a invalid time string: %s', timeString) return 0 end
    local H = fun() or 0
    if H == 0 then c_err('timeString is a invalid time string %s', timeString) return 0 end
    local M = fun() or 0
    if M == 0 then c_err('timeString is a invalid time string %s', timeString) return 0 end
    local S = fun() or 0
    if S == 0 then c_err('timeString is a invalid time string %s', timeString) return 0 end

    return os.time({year=y, month=m, day=d, hour=H,min=M,sec=S})
end

local function convert_to_time_to_glog(_log_str)
	local logs = string.split(_log_str, '|') 
	
	if #logs < 1 then
		c_err("convert_to_json_string: log_str error")		
		return
	end
	
	local log_name = logs[1]
	table.remove(logs, 1)
	
	local table_cfg = log_convert.log_tables[log_name]
	if not table_cfg then   -- log 配置不存在	
		c_err("convert_to_json_string: log config not found, log_name =  " .. log_name)
		return
	end
	
	local table_fields = table_cfg.table_values
	local new_prop = log_name
	for i, v in ipairs(table_fields) do
		
		local value = logs[i]
        if (v.type == "datetime") then
            local timestamp = string2time( value ) + 8 *3600
            if timestamp ~= 0 then
                logs[i] = os.date("!%Y-%m-%d %H:%M:%S", timestamp )
            end
        end
        new_prop = new_prop .. "|" .. logs[i]
	end

	return new_prop
end


function on_log( _server_id, _log_str )
    local sys_log = convert_to_time_to_glog( _log_str )
    --c_err("sys_log: %s", sys_log)
    --c_err("log_str: %s", _log_str)
    c_syslog(sys_log)
    c_backup(_log_str)
	local tga_log = log_convert.convert_to_json_string( _log_str )  -- 转一份发给tga平台
	c_tgalog( tga_log )
end

function on_online( _server_id, _online_num )
    update_gs_online( _server_id, _online_num )
end

function on_timer()
    log_gs_online()
end

function update_gs_online( _server_id, _online_num )
    local curr_time = ostime()
    gs_online_map_[_server_id] = {_online_num, curr_time}
    c_log("update_gs_online, server_id:%d, online_num:%d, curr_time:%d", _server_id, _online_num, curr_time )
end

function log_gs_online()
    local date = osdate("%Y-%m-%d %H:%M:%S")
    local log_str = date
    local curr_time = ostime()
    for server_id, info in pairs( gs_online_map_ ) do
        local old_time = info[2]
        local time_diff = curr_time - old_time
        if time_diff > OVERDUE_PERIOD then
            info[1] = 0
            info[2] = curr_time
            c_err("log_gs_online, overdue! server_id:%d", server_id )
        end
        local online_num = info[1]
        log_str = log_str .."|" .. server_id .. ":" .. online_num
    end
    c_online( log_str )
end

