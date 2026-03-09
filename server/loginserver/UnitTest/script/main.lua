
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
    --package.path = "./?.lc"
    package.path = "./?.lc"
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

require_ex( "luacommon/msg" )
require_ex( "luacommon/utils" )

MESSAGES = require_ex( "luacommon/setting/MESSAGES" )
GM_CMD_CFG = require_ex( "luacommon/setting/GM_CMD_CFG" )

PUSH_MESSAGES = require_ex( "luacommon/setting/PUSH_MESSAGES" )

require_ex( "global" )
require_ex( "luacommon/timer/timers" )
require_ex( "timer/timers" )
require_ex( "db/db_service" )
require_ex( "net/game_login_server" )
require_ex( "luacommon/login_code" )
require_ex( "net/player_login_server" )
require_ex( "luacommon/net/gmclient" )
require_ex( "luacommon/net/http_mng" )
require_ex( "luacommon/splus_sdk/splus_verify" )
require_ex( "luacommon/net/gamesvr_safe" )
require_ex( "luacommon/net/logclient" )
require_ex( "luacommon/net/logclient_tw" )
require_ex( "luacommon/time_event/time_event_log_handle_reset" )


require_ex( "luacommon/gm_cmd" )
require_ex( "gm_cmd" )

time_event_log_handle_reset.init()

