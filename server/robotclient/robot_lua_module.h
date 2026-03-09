#ifndef __GAME_LUA_MODULE__
#define __GAME_LUA_MODULE__

#include "lua_server.h"
#include "app_base.h"
#include "singleton.h"

class RobotLuaSvr : public LuaServer
{
    public:
        enum LuaRef {         
            ROBOT_MESSAGE_HANDLER,
            ON_LOGINSVR_CONNECT,
            ON_GAMESVR_CONNECT,
            ON_BFSVR_CONNECT,
            TIMER_HANDLER,
            ROBOT_USER_ID_HANDLER,
            CTRL_LUA_HANDLER,
            SET_ROBOT_TYPE,
        };

        void register_lua_refs(); 
        void register_class();    
        void register_global();   

        virtual int32_t get_timer_ref() { return TIMER_HANDLER; }
};

#define g_luasvr ( Singleton<RobotLuaSvr>::instance_ptr() )

class LuaModule : public AppClassInterface
{
    public:
        bool app_class_init();    
        bool app_class_destroy(); 
};

#endif // __GAME_LUA__
