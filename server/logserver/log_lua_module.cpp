#include "log.h"
#include "singleton.h"
#include "lar.h"
#include "log_svr_module.h"
#include "log_lua_module.h"
#include "gm_client_module.h"
#include "timer.h"
#include "random_generator.h"

extern LuaServer* lua_timer;

void LogLuaSvr::register_class()
{
    Lunar<LAr>::Register( L_ );  
    Lunar<GMClient>::Register( L_ );  
    Lunar<TimerMng>::Register( L_ );  
	Lunar<RandomGenerator>::Register(L_);
	Lunar<LogSvr>::Register(L_);
}

void LogLuaSvr::register_global()
{
    LuaServer::register_global();

    Lunar<GMClient>::push( L_, g_gmclient, false );
    lua_setglobal( L_, "g_gmclient" );

    Lunar<TimerMng>::push( L_, g_timermng );
    lua_setglobal( L_, "g_timermng" );

	Lunar<LogSvr>::push(L_, g_logsvr, false);
	lua_setglobal(L_, "g_logsvr");
}

void LogLuaSvr::register_lua_refs()
{
    get_table_field_ref( "logserver", "on_log", ON_LOG, LUA_REGISTRYINDEX );
    get_table_field_ref( "logserver", "on_online", ON_ONLINE, LUA_REGISTRYINDEX );
    get_table_field_ref( "gmclient", "message_handler", GMCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "timers", "handle_timer", TIMER_HANDLER, LUA_REGISTRYINDEX );
}

bool LuaModule::app_class_init()
{
    char lua_dir[APP_CFG_NAME_MAX];
    char lua_main_file[APP_CFG_NAME_MAX];
    APP_GET_TABLE( "Lua", 2 );
    APP_GET_STRING( "Path", lua_dir );
    APP_GET_STRING( "MainFile", lua_main_file );
    APP_END_TABLE();

    if (g_luasvr->start_server( lua_dir, lua_main_file ))
    {
        lua_timer = g_luasvr;
        LOG(2)( "===========Lua Module Start===========" );
        LOG(2)( "LuaPath:%s  Main:%s", lua_dir, lua_main_file );
    }
    else
    {
        LOG(2)( "Lua Module Start Failed, LuaPath:%s Main:%s", lua_dir, lua_main_file );
        return false;
    }
    return true;
}

bool LuaModule::app_class_destroy()
{
    LOG(2)( "===========Lua Module Destroy===========" );
    return true;
}

