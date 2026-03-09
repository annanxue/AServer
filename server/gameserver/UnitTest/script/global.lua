
local ttype, jutil, sformat, tinsert, tremove, dbginfo = type, jit.util, string.format, table.insert, table.remove, debug.getinfo

local mark_map = {}
local func_map = {}

local stack_map = {}

local function show_all_func( _name, _func )
    local stats = jutil.stats( _func )
    if stats and stats.mcodeaddr then
        local maddr = stats.mcodeaddr or 0
        local msize = stats.mcodesize or 0
        if not func_map[maddr] then
            c_log( sformat( "[JIT]func name: %s, addr: 0x%x, size: %d", _name, maddr, msize ) )
        end
        func_map[maddr] = true
    end
end

local function mark_all_table( _root )
    for k, v in pairs( _root ) do
        if ttype(v) == "function" then
            show_all_func( k, v )
        elseif ttype(v) == "table" and not mark_map[v] then
            mark_map[v] = true
            mark_all_table( v )
        end
    end
end

local function get_func_name( _info )
    local name = _info.name or "?"
    local linedefined = _info.linedefined
    local source = _info.source
    return sformat( "[%s:%d]", source, linedefined ), name
end

local function update_call( _file, _name )
    local call_info = stack_map[_file]
    if not call_info then
        call_info = { file_ = _file, name_ = _name, call_count_ = 0, ret_count_ = 0, use_ms_ = 0 }
        stack_map[_file] = call_info
    end
    call_info.call_count_ = call_info.call_count_ + 1
    call_info.use_ms_ = call_info.use_ms_ - c_cpu_ms()
end

local function update_return( _file )
    local call_info = stack_map[_file]
    if call_info then
        call_info.ret_count_ = call_info.ret_count_ + 1
        call_info.use_ms_ = call_info.use_ms_ + c_cpu_ms()
    end
end

local function call_hook( _event )
    local func_info = dbginfo( 2 )
    local file, name = get_func_name( func_info )
    if _event == "call" then
        update_call( file, name )
    elseif _event == "return" then
        update_return( file )
    elseif _event == "tail return" then
        --c_debug()
    end
end

function log_all_func_stats()
    mark_map = {}
    mark_all_table( _G )
end

function hook_call()
    stack_map = {}
    debug.sethook( call_hook, "cr" )
end

function unhook_call()
    debug.sethook()
end

function stats_call( _file_name )
    local f = io.open( _file_name, "w+")
    local show_list = {}
    local cmp_func = function( t1, t2 ) return t1.use_ms_ > t2.use_ms_ end
    for k, v in pairs( stack_map ) do
        if v.call_count_ == v.ret_count_ then
            tinsert( show_list, v )
        end
    end
    table.sort( show_list, cmp_func )
    for i, v in ipairs( show_list ) do
        local log = sformat( "\n[%s](%s)\n\t\tcount: %d,\t\ttime: %d ms",  v.file_, v.name_, v.call_count_, v.use_ms_ )
        f:write( log )
    end
    f:close()
end

function on_exit()
    --log_all_func_stats()
end

function on_server_pre_stop()
    c_log("[global](on_server_pre_stop)")
    guild_mng.save_all_guilds()
    hall_mng.gs_on_server_pre_stop()
    event_one_gold_lucky_draw.save_data()
end

