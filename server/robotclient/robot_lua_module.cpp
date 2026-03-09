#include "robot_lua_module.h"
#include "robot_client_module.h"
#include "log.h"
#include "llog.h"
#include "lmisc.h"
#include "timer.h"
#include "lar.h"
#include "ctrl.h"
#include "player.h"
#include "bullet.h"
#include "trigger.h"
#include "app_curl_module.h"
#include "scene_obj_utils.h"
#include "random_generator.h"

extern LuaServer* lua_timer;

void RobotLuaSvr::register_lua_refs()
{
    get_table_field_ref( "robotclient", "message_handler", ROBOT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "robotclient", "on_loginsvr_connect", ON_LOGINSVR_CONNECT, LUA_REGISTRYINDEX );
    get_table_field_ref( "robotclient", "on_gamesvr_connect", ON_GAMESVR_CONNECT, LUA_REGISTRYINDEX );
    get_table_field_ref( "robotclient", "on_bfsvr_connect", ON_BFSVR_CONNECT, LUA_REGISTRYINDEX );
    get_table_field_ref( "timers", "handle_timer", TIMER_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "robotclient", "set_user_id_begin_num", ROBOT_USER_ID_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "robotclient", "set_robot_type", SET_ROBOT_TYPE, LUA_REGISTRYINDEX );
    get_table_field_ref( "ctrl_mng", "ctrl_lua_handler", CTRL_LUA_HANDLER, LUA_REGISTRYINDEX );

    Ctrl::set_lua_server( this );
    Ctrl::set_lua_regid( CTRL_LUA_HANDLER );
}

void RobotLuaSvr::register_class()
{
    Lunar<LAr>::Register( L_ );  
    Lunar<RobotClient>::Register( L_ );  
    Lunar<TimerMng>::Register( L_ );  
    Lunar<Player>::Register( L_ );  
    Lunar<Monster>::Register( L_ );  
    Lunar<Bullet>::Register( L_ );  
    Lunar<Trigger>::Register( L_ );  
    Lunar<HttpMng>::Register( L_ );
    Lunar<RandomGenerator>::Register( L_ );
}

void RobotLuaSvr::register_global()
{
    LuaServer::register_global();
    register_scene_obj_utils( L_ );

    Lunar<RobotClient>::push( L_, g_robotclient, false );
    lua_setglobal( L_, "g_robotclient" );

    Lunar<TimerMng>::push( L_, g_timermng );
    lua_setglobal( L_, "g_timermng" );

    Lunar<HttpMng>::push( L_, g_http_mng );
    lua_setglobal( L_, "g_http_mng" );
}

bool LuaModule::app_class_init()
{
    char lua_dir[APP_CFG_NAME_MAX];
    char lua_main_file[APP_CFG_NAME_MAX];
    APP_GET_TABLE("Lua",2);
    APP_GET_STRING("Path", lua_dir);
    APP_GET_STRING("MainFile", lua_main_file);
    APP_END_TABLE();

    if( g_luasvr->start_server( lua_dir, lua_main_file ) ) {
        lua_timer = g_luasvr;
        LOG(2)( "===========Lua Module Start===========");
        LOG(2)( "LuaPath:%s  Main:%s", lua_dir, lua_main_file );
    }
    else {
        ERR(2)( "Lua Module Start Failed, LuaPath:%s Main:%s", lua_dir, lua_main_file );
        return false;
    }
    return true;
}

bool LuaModule::app_class_destroy()
{
    LOG(2)( "===========Lua Module Destroy===========");
    return true;
}

