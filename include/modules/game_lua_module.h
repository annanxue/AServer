#ifndef ASERVER_GAME_LUA_MODULE_H
#define ASERVER_GAME_LUA_MODULE_H

#include "define.h"
#include "app_base.h"
#include "lua_server.h"
#include "singleton.h"

namespace AServer {

class GameLuaSvr : public LuaServer {
public:
    enum LuaRef {
        DBCLIENT_MESSAGE_HANDLER = 0,
        GAMESERVER_MESSAGE_HANDLER = 1,
        LOGINCLIENT_MESSAGE_HANDLER = 2,
        GMCLIENT_MESSAGE_HANDLER = 3,
        TIMER_HANDLER = 4,
        BFCLIENT_MESSAGE_HANDLER = 5,
        CAN_BF_SAVE_PLAYER = 6,
        CTRL_LUA_HANDLER = 7,
        KICK_ALL_PLAYER_HANDLER = 8,
        ON_SERVER_PRE_STOP = 9,
        STATE_MSG = 10,
        ON_EXIT = 11,
        SET_MAX_USER_NUM = 12,
        LOGCLIENT_MESSAGE_HANDLER = 13,
        GM_SVR_MESSAHE_HANDLER = 14,
        MAX_LUA_REF
    };

    void register_lua_refs() override;
    void register_class() override;
    void register_global() override;
    void on_exit();
    int32_t get_timer_ref() override { return TIMER_HANDLER; }
};

class LuaModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

}

#endif
