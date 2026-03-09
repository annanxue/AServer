#ifndef __LOG_CLIENT_MODULE__
#define __LOG_CLIENT_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class LogClient : public NetMng
{
public:
    LogClient();
    ~LogClient();
public:
    virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
private:
    void init_func_map();
    void on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    void on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
    void send_certify();
private:
    void (LogClient::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
public:
    static const char className[];
    static Lunar<LogClient>::RegType methods[];
    static const bool gc_delete_;
    char certify_code_[MAX_CERTIFY_CODE];
};

class LogClientModule : public AppClassInterface
{
public:
    bool app_class_init();
    bool app_class_destroy();
};

#define g_logclient Singleton<LogClient>::instance_ptr() 

#endif
