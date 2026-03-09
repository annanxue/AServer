#ifndef __LOGIN_LUA_MODULE_H
#define __LOGIN_LUA_MODULE_H

//#include "coreclient.h"
//#include "rpc.h"
#include "lunar.h"
#include "database.h"
#include "app_base.h"
#include "mymap32.h"
#include "lua_server.h"
#include "llog.h"
#include "buffer.h"

int32_t c_read_mysql( lua_State* _L );

class LuaServerModule:public AppClassInterface
{
public:
    bool app_class_init();
    bool app_class_destroy();
};

class LoginLuaSvr: public LuaServer
{
public:
    struct SqlTbl{
        char host[APP_CFG_NAME_MAX];
        char user[APP_CFG_NAME_MAX];
        char passwd[APP_CFG_NAME_MAX];
        char port[APP_CFG_NAME_MAX];
        char dbname[APP_CFG_NAME_MAX];
    };

    MyMapStr mysql_cfg_;

    enum LuaRef
    {
        ON_EXIT,
        GAME_LOGIN_MESSAGE_HANDLER,
        PLAYER_LOGIN_MESSAGE_HANDLER,
        TIMER_HANDLER,
        GMCLIENT_MESSAGE_HANDLER,
        LOGCLIENT_MESSAGE_HANDLER
    };
    LoginLuaSvr();
    ~LoginLuaSvr();
    virtual void        register_lua_refs();
    virtual void        register_class();
    virtual int32_t     get_timer_ref(){return TIMER_HANDLER;}
    virtual void        register_global();

    bool read_sql_cfg( const char* _cfg );

private:
    bool read_sql_tbl( lua_State* _L, const char* _name );
};

#define g_luasvr Singleton<LoginLuaSvr>::instance_ptr()

#endif
