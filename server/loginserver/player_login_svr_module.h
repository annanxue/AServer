#ifndef __PLAYER_LOGIN_SVR_MODULE_H__
#define __PLAYER_LOGIN_SVR_MODULE_H__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class PlayerLoginSvr : public NetMng
{
    public:
        PlayerLoginSvr();
        ~PlayerLoginSvr();
    public:
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
        void set_android_version( int _version ) { android_version_ = _version; }
        void set_ios_version( int _version ) { ios_version_ = _version; }
    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );

    private:
        void (PlayerLoginSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    private:
        int android_version_;
        int ios_version_;
    public:
        static const char className[];
        static Lunar<PlayerLoginSvr>::RegType methods[];
        static const bool gc_delete_;
        char gamesvr_config_url_[APP_CFG_NAME_MAX];
    public:
	    int	c_get_android_version( lua_State* _L );
	    int	c_get_ios_version( lua_State* _L );
	    int	c_get_gamesvr_config_url( lua_State* _L );
        int c_reset_client_version( lua_State* _L);
        int c_get_player_ip( lua_State* _L );
};

class PlayerLoginSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_player_login_svr Singleton<PlayerLoginSvr>::instance_ptr() 

#endif
