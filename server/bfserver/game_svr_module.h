#ifndef __GAME_SVR_MODULE__
#define __GAME_SVR_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class GameSvr : public NetMng
{
    public:
        GameSvr();
        ~GameSvr();
    public:
        void set_server_ip_to_client( const char* _ip_to_client );
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
        void kick_player(DPID dpid);
        void kick_all_player();

    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
    private:
        void (GameSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );

    public:
        static const char className[];
        static Lunar<GameSvr>::RegType methods[];
        static const bool gc_delete_;

        int32_t c_get_server_id( lua_State* _L );
        int32_t c_get_server_ip( lua_State* _L );
        int32_t c_get_server_ip_to_client( lua_State* _L );
        int32_t c_get_server_port( lua_State* _L );
        int32_t c_get_user_num( lua_State* _L );
        int32_t c_kick_player( lua_State* _L );
        int32_t	c_get_player_ip( lua_State* _L );

    private:
        char server_ip_to_client_[APP_CFG_NAME_MAX];
};

class GameSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_gamesvr Singleton<GameSvr>::instance_ptr() 

#endif
