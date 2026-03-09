#ifndef __DB_CLIENT_MODULE__
#define __DB_CLIENT_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class DBClient : public NetMng
{
    public:
        DBClient();
        ~DBClient();
    public:
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
        void send_last_msg();
    private:
        void init_func_map();
        void on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void send_exec_sql( const char* sql );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
        void send_register_server_id();
    private:
        void (DBClient::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    public:
        int32_t  c_is_connected( lua_State* _L );

        static const char className[];
        static Lunar<DBClient>::RegType methods[];
        static const bool gc_delete_;
};

class DBClientModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_dbclient Singleton<DBClient>::instance_ptr() 

#endif
