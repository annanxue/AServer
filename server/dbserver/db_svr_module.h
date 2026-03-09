#ifndef __DB_SVR_MODULE__
#define __DB_SVR_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class DbSvr : public NetMng
{
    public:
        DbSvr();
        ~DbSvr();
    public:
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_register_server_id( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_gameserver_last_msg( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );

    private:
        void (DbSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    public:
        static const char className[];
        static Lunar<DbSvr>::RegType methods[];
        static const bool gc_delete_;

        int32_t c_get_server_id( lua_State* _L );
};

class DbSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_dbsvr Singleton<DbSvr>::instance_ptr() 

#endif
