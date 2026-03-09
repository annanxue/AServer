#include "game_lua_module.h"
#include "game_svr_module.h"
#include "app_curl_module.h"
#include "db_client_module.h"
#include "login_client_module.h"
#include "bf_client_module.h"
#include "gm_client_module.h"
#include "log_client_module.h"
#include "log.h"
#include "llog.h"
#include "lmisc.h"
#include "timer.h"
#include "lar.h"
#include "player.h"
#include "ctrl.h"
#include "player_mng.h"
#include "bullet.h"
#include "missile.h"
#include "trigger.h"
#include "scene_obj_utils.h"
#include "component_ai.h"
#include "camp_mng.h"
#include "curve_mng.h"
#include "magic_area.h"
#include "pick_point.h"
#include "item.h"
#include "random_generator.h"
#include "gm_svr_module.h"

extern LuaServer* lua_timer;

void GameLuaSvr::register_lua_refs()
{
    get_table_field_ref( "timers", "handle_timer", TIMER_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "dbclient", "message_handler", DBCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "bfclient", "can_bf_save_player", CAN_BF_SAVE_PLAYER, LUA_REGISTRYINDEX );
    get_table_field_ref( "gamesvr", "message_handler", GAMESERVER_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "loginclient", "message_handler", LOGINCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "gmclient", "message_handler", GMCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "bfclient", "message_handler", BFCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "ctrl_mng", "ctrl_lua_handler", CTRL_LUA_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "player_mng", "kick_all_player", KICK_ALL_PLAYER_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "states", "state_msg", STATE_MSG, LUA_REGISTRYINDEX );
    get_table_field_ref( "_G", "on_server_pre_stop", ON_SERVER_PRE_STOP, LUA_REGISTRYINDEX );
    get_table_field_ref( "_G", "on_exit", ON_EXIT, LUA_REGISTRYINDEX );
    get_table_field_ref( "login_mng", "set_max_user_num", SET_MAX_USER_NUM, LUA_REGISTRYINDEX ); 
    get_table_field_ref( "logclient", "message_handler", LOGCLIENT_MESSAGE_HANDLER, LUA_REGISTRYINDEX );
    get_table_field_ref( "gmsvr", "message_handler", GM_SVR_MESSAHE_HANDLER, LUA_REGISTRYINDEX );

    Ctrl::set_lua_server( this );
    Ctrl::set_lua_regid( CTRL_LUA_HANDLER );
}

void GameLuaSvr::register_class()
{
    Lunar<LAr>::Register( L_ );  
    Lunar<DBClient>::Register( L_ );  
    Lunar<LoginClient>::Register( L_ );  
    Lunar<GMClient>::Register( L_ );  
    Lunar<LogClient>::Register( L_ );  
    Lunar<GameSvr>::Register( L_ );  
    Lunar<TimerMng>::Register( L_ );  
    Lunar<BFClient>::Register( L_ );  
    Lunar<Player>::Register( L_ );  
	Lunar<Monster>::Register( L_ );  
	Lunar<Trigger>::Register( L_ );  
    Lunar<PlayerMng>::Register( L_ );  
    Lunar<Bullet>::Register( L_ );  
    Lunar<Missile>::Register( L_ );  
    Lunar<AI>::Register( L_ );  
    Lunar<CampMng>::Register( L_ );
    Lunar<CurveMng>::Register( L_ );
    Lunar<MagicArea>::Register( L_ );
    Lunar<PickPoint>::Register( L_ );
    Lunar<Item>::Register( L_ );
    Lunar<HttpMng>::Register( L_ );
    Lunar<RandomGenerator>::Register( L_ );
    Lunar<GmSvr>::Register( L_ );  
}

void GameLuaSvr::register_global()
{
    LuaServer::register_global();
    register_scene_obj_utils(L_);

    Lunar<BFClient>::push( L_, g_bfclient, false );
    lua_setglobal( L_, "g_bfclient" );

    Lunar<DBClient>::push( L_, g_dbclient, false );
    lua_setglobal( L_, "g_dbclient" );

    Lunar<GameSvr>::push( L_, g_gamesvr, false );
    lua_setglobal( L_, "g_gamesvr" );

    Lunar<LoginClient>::push( L_, g_loginclient, false );
    lua_setglobal( L_, "g_loginclient" );

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

    Lunar<GmSvr>::push( L_, g_gmsvr, false );
    lua_setglobal( L_, "g_gmsvr" );
}

void GameLuaSvr::on_exit()
{
    if( L_ )
    {
        get_lua_ref( ON_EXIT );
        if( llua_call( L_, 0, 0, 0 ) )
            llua_fail( L_, __FILE__, __LINE__ );
    }
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
    g_luasvr->on_exit();
    LOG(2)( "===========Lua Module Destroy===========");
    return true;
}

