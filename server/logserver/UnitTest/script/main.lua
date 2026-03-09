
function require_ex( _mname )
    package.loaded[_mname] = nil
    return require( _mname )    
end

------------------------------------------------------------------------------------------
--                              初始化LuaJIT
------------------------------------------------------------------------------------------
if g_debug then
    package.path = "./?.lua;./?.lc"
else
    package.path = "./?.lua;./?.lc"
end

require_ex( "jit/opt" ) 

if g_debug then
    if jit then
        jit.debug(2)
        jit.opt.start(3)
        c_log( "++++++++++++++++++LUAJIT(v1) DEBUG OPT LEVEL 3+++++++++++++++++++" )
    end
else
    if jit then
        jit.debug(0)
        jit.opt.start(3)
        c_log( "++++++++++++++++++LUAJIT(v1) RELEASE OPT LEVEL 3+++++++++++++++++++" )
    end
end

------------------------------------------------------------------------------------------
--                              加载所有模块
------------------------------------------------------------------------------------------

GM_CMD_CFG = require_ex( "luacommon/setting/GM_CMD_CFG" )
LOG_CONFIG = require_ex( "luacommon/setting/LOG_CONFIG" )

require_ex( "luacommon/gm_cmd" )
require_ex( "luacommon/msg" )
require_ex( "luacommon/timer/timers" )
require_ex( "luacommon/net/gmclient" )
require_ex( "net/logserver" )
require_ex( "luacommon/net/log_convert" )
require_ex( "luacommon/utils" )
require_ex( "luacommon/time_event/time_event_log_handle_reset" )

math.randomseed( os.time() )

time_event_log_handle_reset.init()



