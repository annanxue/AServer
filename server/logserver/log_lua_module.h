#ifndef __LOG_LUA_MODULE__
#define __LOG_LUA_MODULE__

#include "lunar.h"
#include "app_base.h"
#include "mymap32.h"
#include "lua_server.h"
#include "llog.h"
#include "buffer.h"

class LuaModule:public AppClassInterface
{
public:
    bool app_class_init();
    bool app_class_destroy();
};

class LogLuaSvr: public LuaServer
{
public:
    enum LuaRef
    {
        ON_LOG,
        ON_ONLINE,
        GMCLIENT_MESSAGE_HANDLER,
        TIMER_HANDLER,
    };
    virtual void        register_lua_refs();
    virtual void        register_class();
    virtual void        register_global();
    virtual int32_t     get_timer_ref(){return TIMER_HANDLER;}
};

#define g_luasvr Singleton<LogLuaSvr>::instance_ptr()

#endif // __LOG_LUA_MODULE__
