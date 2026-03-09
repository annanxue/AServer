#ifndef __LOG_SVR_MODULE__
#define __LOG_SVR_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class LogSvr : public NetMng
{
    public:
        LogSvr();
        ~LogSvr();
    public:
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_certify( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_log( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_online( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );

		int32_t c_get_server_ip(lua_State* _L);
		int32_t c_get_server_port(lua_State* _L);

    private:
        void (LogSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    public:
        static const char className[];
        static Lunar<LogSvr>::RegType methods[];
        static const bool gc_delete_;
        static char log_str_[];
        char certify_code_[MAX_CERTIFY_CODE];
};

class LogSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_logsvr Singleton<LogSvr>::instance_ptr() 

#endif
