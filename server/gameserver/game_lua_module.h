#ifndef __GAME_LUA_MODULE__
#define __GAME_LUA_MODULE__

#include "lua_server.h"
#include "app_base.h"
#include "singleton.h"

class GameLuaSvr : public LuaServer
{
    public:
        enum LuaRef {         
            DBCLIENT_MESSAGE_HANDLER, 
            GAMESERVER_MESSAGE_HANDLER, 
            LOGINCLIENT_MESSAGE_HANDLER, 
            GMCLIENT_MESSAGE_HANDLER, 
            TIMER_HANDLER,
            BFCLIENT_MESSAGE_HANDLER,
            CAN_BF_SAVE_PLAYER,
            CTRL_LUA_HANDLER,
            KICK_ALL_PLAYER_HANDLER,
            ON_SERVER_PRE_STOP,
            STATE_MSG, 
            ON_EXIT,
            SET_MAX_USER_NUM,
            LOGCLIENT_MESSAGE_HANDLER,
            GM_SVR_MESSAHE_HANDLER, 
        };

        void register_lua_refs(); 
        void register_class();    
        void register_global();   
        void on_exit();
        virtual int32_t     get_timer_ref(){return TIMER_HANDLER;}
};

#define g_luasvr ( Singleton<GameLuaSvr>::instance_ptr() )

class LuaModule : public AppClassInterface
{
    public:
        bool app_class_init();    
        bool app_class_destroy(); 
};

#endif // __GAME_LUA__
