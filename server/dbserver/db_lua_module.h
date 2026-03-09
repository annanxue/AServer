#ifndef __DB_LUA_MODULE_H
#define __DB_LUA_MODULE_H

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

class DBLuaSvr: public LuaServer
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
        SET_FRAME,
        SET_GAME_DPID,
        GET_INIT_RESULT,
        DB_MESSAGE_HANDLER,
        GMCLIENT_MESSAGE_HANDLER,
        TIMER_HANDLER,
    };
    DBLuaSvr();
    ~DBLuaSvr();
    virtual void        register_lua_refs();
    virtual void        register_class();
    virtual void        register_global();

    bool read_sql_cfg( const char* _cfg );

    void set_frame( uint32_t _frame ){
        get_lua_ref( SET_FRAME );
        lua_pushnumber( L_, _frame );
        if(llua_call( L_, 1, 0, 0 )){
            llua_fail( L_, __FILE__, __LINE__ );
        }
    }

    void set_game_dpid( DPID _dpid ){
        get_lua_ref( SET_GAME_DPID );
        lua_pushnumber( L_, _dpid );
        if(llua_call( L_, 1, 0, 0 )){
            llua_fail( L_, __FILE__, __LINE__ );
        }
    }

    virtual int32_t     get_timer_ref(){return TIMER_HANDLER;}

private:
    bool read_sql_tbl( lua_State* _L, const char* _name );
};

#define g_luasvr Singleton<DBLuaSvr>::instance_ptr()

#endif
