#include "bf_lua_module.h"
#include "bf_svr_module.h"
#include "game_svr_module.h"
#include "gm_client_module.h"
#include "log.h"
#include "llog.h"
#include "lmisc.h"
#include "lar.h"
#include "timer.h"
#include "ctrl.h"
#include "player.h"
#include "player_mng.h"
#include "scene_obj_utils.h"
#include "component_ai.h"
#include "camp_mng.h"
#include "bullet.h"
#include "missile.h"
#include "trigger.h"
#include "camp_mng.h"
#include "curve_mng.h"
#include "magic_area.h"
#include "pick_point.h"
#include "item.h"
#include "app_curl_module.h"
#include "log_client_module.h"
#include "random_generator.h"


extern LuaServer* lua_timer;

void BFLuaSvr::register_lua_refs()
{
    get_table_field_ref( "bfsvr", "message_handler", BFSVR_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "gamesvr", "message_handler", GAMESERVER_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "gmclient", "message_handler", GMCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "timers", "handle_timer", TIMER_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "states", "state_msg", STATE_MSG, LUA_REGISTRYINDEX );
    get_table_field_ref( "ctrl_mng", "ctrl_lua_handler", CTRL_LUA_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "player_mng", "kick_all_player", KICK_ALL_PLAYER_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "logclient", "message_handler", LOGCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );

    Ctrl::set_lua_server( this );
    Ctrl::set_lua_regid( CTRL_LUA_HANDLER );
}

void BFLuaSvr::register_class()
{
    Lunar<LAr>::Register( L_ );  
    Lunar<BFSvr>::Register( L_ );  
    Lunar<GameSvr>::Register( L_ );  
    Lunar<GMClient>::Register( L_ );  
    Lunar<TimerMng>::Register( L_ );
    Lunar<Player>::Register( L_ );  
    Lunar<PlayerMng>::Register( L_ );  
    Lunar<CampMng>::Register( L_ );
    Lunar<CurveMng>::Register( L_ );
    Lunar<Monster>::Register( L_ );
	Lunar<Trigger>::Register( L_ );  
    Lunar<Bullet>::Register( L_ );  
    Lunar<Missile>::Register( L_ );  
    Lunar<AI>::Register( L_ );  
    Lunar<MagicArea>::Register( L_ );
    Lunar<PickPoint>::Register( L_ );
    Lunar<Item>::Register( L_ );
    Lunar<HttpMng>::Register( L_ );
    Lunar<LogClient>::Register( L_ );
    Lunar<RandomGenerator>::Register( L_ );
}

void BFLuaSvr::register_global()
{
    LuaServer::register_global();
    register_scene_obj_utils(L_);

    Lunar<BFSvr>::push( L_, g_bfsvr, false );
    lua_setglobal( L_, "g_bfsvr" );

    Lunar<GameSvr>::push( L_, g_gamesvr, false );
    lua_setglobal( L_, "g_gamesvr" );

    Lunar<GMClient>::push( L_, g_gmclient, false );
    lua_setglobal( L_, "g_gmclient" );

    Lunar<TimerMng>::push( L_, g_timermng );
    lua_setglobal( L_, "g_timermng" );

    Lunar<PlayerMng>::push( L_, g_playermng );
    lua_setglobal( L_, "g_playermng" );

    Lunar<CampMng>::push( L_, g_campmng );
    lua_setglobal( L_, "g_campmng" );
    
    Lunar<CurveMng>::push( L_, g_curvemng );
    lua_setglobal( L_, "g_curvemng" );

    Lunar<HttpMng>::push( L_, g_http_mng );
    lua_setglobal( L_, "g_http_mng" );
    
    Lunar<LogClient>::push( L_, g_logclient );
    lua_setglobal( L_, "g_logclient" );

}

bool LuaModule::app_class_init()
{
    char lua_dir[APP_CFG_NAME_MAX];
    char lua_main_file[APP_CFG_NAME_MAX];
    APP_GET_TABLE("Lua",2);
    APP_GET_STRING("Path", lua_dir);
    APP_GET_STRING("MainFile", lua_main_file);
    APP_END_TABLE();

    Ctrl::init_lua_msg();
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

