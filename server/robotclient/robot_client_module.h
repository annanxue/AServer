#ifndef __GAME_SVR_MODULE__
#define __GAME_SVR_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"
#include "mymap32.h"

using namespace FF_Network;

class RobotClient : public NetMng
{
    public:
        RobotClient();
        ~RobotClient();
    public:
        int init_multi_client( int _max_con, int _timeval = 100 );
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
        void set_loginsvr_addr( const char* _ip, int32_t _port );
        void set_gamesvr_id( int32_t _gamesvr_id );
        void set_gamesvr_addr( const char* _ip, int32_t _port );
        void set_user_id_begin_num( int _begin_num );
        void set_robot_type( int _type );
        void set_login_type( const char* _login_type ){ strncpy( login_type_, _login_type, sizeof( login_type_ ) ); }
    private:
        void init_func_map();
        void on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
        void lua_on_loginsvr_connect( lua_State* _L, DPID _dpid );
        void lua_on_gamesvr_connect( lua_State* _L, DPID _dpid );
        void lua_on_bfsvr_connect( lua_State* _L, DPID _dpid, const char* _ip, int32_t _port );
    private:
        void (RobotClient::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );

        char loginsvr_ip_[APP_CFG_NAME_MAX];
        int32_t loginsvr_port_;
        int32_t gamesvr_id_;
        char gamesvr_ip_[APP_CFG_NAME_MAX];
        int32_t gamesvr_port_;
        char login_type_[APP_CFG_NAME_MAX];

        MyMap32 nonreconnect_dpids_;  // 该集合内的连接在断开后不会自动重连
    public:
        static const char className[];
        static Lunar<RobotClient>::RegType methods[];
        static const bool gc_delete_;

        int32_t c_get_gamesvr_id( lua_State* _L );
        int32_t c_connect_gamesvr( lua_State* _L );
        int32_t c_connect_bfsvr( lua_State* _L );
        int32_t c_disconnect( lua_State* _L );
        int32_t c_add_nonreconnect_dpid( lua_State* _L );
        int32_t c_get_login_type( lua_State* _L );
};

class RobotClientModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_robotclient Singleton<RobotClient>::instance_ptr() 

#endif
