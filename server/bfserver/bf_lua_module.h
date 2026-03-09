#ifndef __BF_LUA_MODULE__
#define __BF_LUA_MODULE__

#include "lua_server.h"
#include "app_base.h"
#include "singleton.h"

class BFLuaSvr : public LuaServer
{
    public:
        enum LuaRef {         
            GAMESERVER_MESSAGE_HANDLER, 
            BFSVR_MESSAGE_HANDLER,
            GMCLIENT_MESSAGE_HANDLER,
            TIMER_HANDLER,
            STATE_MSG,
            KICK_ALL_PLAYER_HANDLER,
            CTRL_LUA_HANDLER,
            LOGCLIENT_MESSAGE_HANDLER
        };

        void register_lua_refs(); 
        void register_class();    
        void register_global();   
        int get_timer_ref(){return TIMER_HANDLER;}
};

#define g_luasvr ( Singleton<BFLuaSvr>::instance_ptr() )

class LuaModule : public AppClassInterface
{
    public:
        bool app_class_init();    
        bool app_class_destroy(); 
};

#endif //
