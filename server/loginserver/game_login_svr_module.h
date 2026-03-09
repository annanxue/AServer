#ifndef __GAME_LOGIN_LUA_MODULE_H__
#define __GAME_LOGIN_SVR_MODULE_H__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class GameLoginSvr : public NetMng
{
    public:
        GameLoginSvr();
        ~GameLoginSvr();
    public:
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );

        int32_t c_get_server_ip( lua_State* _L );
        int32_t c_get_server_port( lua_State* _L );
    public:
	    int32_t	c_get_platform_tag( lua_State* _L );
	    int32_t	c_get_game_id( lua_State* _L );
        char platform_tag_[APP_CFG_NAME_MAX];
        char game_id_[APP_CFG_NAME_MAX];
    private:
        void (GameLoginSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    public:
        static const char className[];
        static Lunar<GameLoginSvr>::RegType methods[];
        static const bool gc_delete_;

};

class GameLoginSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_game_login_svr Singleton<GameLoginSvr>::instance_ptr() 

#endif
