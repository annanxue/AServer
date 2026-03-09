#include "log.h"
#include "singleton.h"
#include "lar.h"
#include "game_login_svr_module.h"
#include "player_login_svr_module.h"
#include "app_curl_module.h"
#include "login_lua_module.h"
#include "gm_client_module.h"
#include "log_client_module.h"
#include "timer.h"
#include "random_generator.h"

extern LuaServer* lua_timer;

int32_t c_read_mysql( lua_State* _L ){
    lcheck_argc( _L, 1 ); 
    size_t l;
    const char* tbl_name = luaL_checklstring( _L, 1, &l );

    intptr_t iptr = 0;
    if( !g_luasvr->mysql_cfg_.find( tbl_name, iptr ) ) {
        ERR(2)( "[loginluasvr] (c_read_mysql) invalid table name:%s", tbl_name );
        return 0;
    }

    LoginLuaSvr::SqlTbl* cfg = ( LoginLuaSvr::SqlTbl* )iptr;
    lua_pushstring( _L, cfg->host );
    lua_pushstring( _L, cfg->user );
    lua_pushstring( _L, cfg->passwd );
    lua_pushstring( _L, cfg->port );
    lua_pushstring( _L, cfg->dbname );

    return 5;
}

LoginLuaSvr::LoginLuaSvr() {
    mysql_cfg_.init( 4, 16 , "mysql_cfg");
}

LoginLuaSvr::~LoginLuaSvr()
{
    if (L_){
        get_lua_ref( ON_EXIT );
        if( llua_call( L_, 0, 0, 0 ) )
            llua_fail( L_, __FILE__, __LINE__ );
    }

    StrNode* node = mysql_cfg_.first();
    if( node ) delete (SqlTbl*)node->val;
    for( int32_t i=1; i<mysql_cfg_.count(); i++ ){
        node = mysql_cfg_.next( node );
        if( node ) delete (SqlTbl*)node->val;
    }
}

bool LuaServerModule::app_class_init(){
    char lua_dir[APP_CFG_NAME_MAX];
    char lua_main_file[APP_CFG_NAME_MAX];
    APP_GET_TABLE("Lua",2);
    APP_GET_STRING("path", lua_dir );
    APP_GET_STRING("main_file", lua_main_file );
    APP_END_TABLE();

    char sql_cfg_file[APP_CFG_NAME_MAX];
    APP_GET_STRING( "MysqlCfg", sql_cfg_file );

    if( !g_luasvr->read_sql_cfg( sql_cfg_file ) ){
        ERR(2)( "[loginluasvr]%s:%d load MysqlConfig err\n", __FILE__, __LINE__ );
        exit(0);
    }
   
    if (g_luasvr->start_server( lua_dir, lua_main_file )){
        lua_timer = g_luasvr;
        LOG(2)( "===========Lua Module Start===========");
        LOG(2)( "LuaPath:%s  Main:%s", lua_dir, lua_main_file );
    }
    else {
        LOG(2)( "Lua Module Start Failed, LuaPath:%s Main:%s", lua_dir, lua_main_file );
        return false;
    }
    return true;
}

bool LuaServerModule::app_class_destroy(){
    //Singleton<LoginLuaSvr>::release();
    LOG(2)( "===========Lua Module Destroy===========");
    return true;
}

bool LoginLuaSvr::read_sql_cfg( const char* _cfg ){
    lua_State *L = lua_open();
    if( Lua::do_file( L, _cfg ) ){
        lua_close(L);
        return false;
    }

    lua_pushnil(L);
    while(lua_next(L, LUA_GLOBALSINDEX))
    {
        if( lua_isnil( L, -1 ) || !lua_istable( L, -1 )
                || lua_isnil( L, -2 ) || !lua_isstring( L, -2 )){
            lua_pop(L, 1);
            continue;
        }
        const char *name = lua_tostring(L, -2);
        if (name == NULL){
            lua_pop(L, 1);
            continue;
        }
        //lua stack 
        //-1: global table
        //-2: global table name
        read_sql_tbl(L, name);
        lua_pop( L, 1 );
    }
    lua_close( L );

    return true;
}

bool LoginLuaSvr::read_sql_tbl( lua_State* _L, const char* _name ){
//    if( Lua::get_table( _L, LUA_GLOBALSINDEX, _name ) == -1 ) {
//        return false;
//    }
    SqlTbl* cfg = new SqlTbl;
    mysql_cfg_.set( _name, (intptr_t)cfg ); 
    Lua::get_table_string( _L, -1, "host", cfg->host, APP_CFG_NAME_MAX );
    Lua::get_table_string( _L, -1, "user", cfg->user, APP_CFG_NAME_MAX );
    Lua::get_table_string( _L, -1, "passwd", cfg->passwd, APP_CFG_NAME_MAX );
    Lua::get_table_string( _L, -1, "port", cfg->port, APP_CFG_NAME_MAX );
    Lua::get_table_string( _L, -1, "dbname", cfg->dbname, APP_CFG_NAME_MAX );
    return true;
}

void LoginLuaSvr::register_class()
{
    Lunar<DataBase>::Register( L_ );
    Lunar<LAr>::Register( L_ );
    Lunar<PlayerLoginSvr>::Register( L_ );
    Lunar<GameLoginSvr>::Register( L_ );
    Lunar<TimerMng>::Register( L_ );
    Lunar<HttpMng>::Register( L_ );
    Lunar<GMClient>::Register( L_ );  
    Lunar<LogClient>::Register( L_ );
    Lunar<RandomGenerator>::Register( L_ );
}

void LoginLuaSvr::register_global()
{
    LuaServer::register_global();

    Lunar<GameLoginSvr>::push( L_, g_game_login_svr, false );
    lua_setglobal( L_, "g_game_login_svr" );

    Lunar<PlayerLoginSvr>::push( L_, g_player_login_svr, false );
    lua_setglobal( L_, "g_player_login_svr" );

    Lunar<TimerMng>::push( L_, g_timermng );
    lua_setglobal( L_, "g_timermng" );

    Lunar<HttpMng>::push( L_, g_http_mng );
    lua_setglobal( L_, "g_http_mng" );

    Lunar<GMClient>::push( L_, g_gmclient, false );
    lua_setglobal( L_, "g_gmclient" );

    lua_register( L_, "c_read_mysql", c_read_mysql );

    Lunar<LogClient>::push( L_, g_logclient );
    lua_setglobal( L_, "g_logclient" );
}

void LoginLuaSvr::register_lua_refs()
{
    get_table_field_ref( "_G", "on_exit", ON_EXIT, LUA_REGISTRYINDEX );
    get_table_field_ref( "timers", "handle_timer", TIMER_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "game_login_server", "message_handler", GAME_LOGIN_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "player_login_server", "message_handler", PLAYER_LOGIN_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "gmclient", "message_handler", GMCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "logclient", "message_handler", LOGCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
}

