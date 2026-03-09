#ifndef __CONNECT_THREAD_H__
#define __CONNECT_THREAD_H__

#include "autolock.h"
#include "mylist.h"
#include "mymap32.h"
#include "thread.h"
#include "netmng.h"
#include "singleton.h"

using namespace FF_Network;

struct ConnInfo
{
    list_head   link_;
    bool        connected_;
    char        ip_[16];
    uint16_t    port_;
};

class ConnectThread : public Thread
{
public:
    ConnectThread() : netmng_( 0 ), crypt_(false), time_wait_(100), sleep_time_(1000) { thread_name_ = "ConnectThread"; }
    ~ConnectThread() {}

    void init( NetMng* _netmng, char* _ip, uint16_t _port, bool _crypt = false, int _time_wait = 100, int _sleep_time = 1000 );
    void run();
    void on_conn_disconnect();
    bool is_connected(){return conn_info_.connected_;}
    void reset_connect_info( const char* _ip, uint16_t _port );

protected:
    NetMng*     netmng_;
    ConnInfo    conn_info_;
    bool        crypt_;
    int         time_wait_;
    int         sleep_time_;
    bool        is_conn_info_reset_;
};

//ConnectThread各个实例
#define g_connect_thread ( Singleton<ConnectThread, 0>::instance_ptr() )
#define g_login_connect_thread ( Singleton<ConnectThread, 1>::instance_ptr() )
#define g_gm_connect_thread ( Singleton<ConnectThread, 2>::instance_ptr() )
#define g_log_connect_thread ( Singleton<ConnectThread, 3>::instance_ptr() )

class MultiConnectThread : public Thread
{
public:
    MultiConnectThread();
    ~MultiConnectThread();

    void init( NetMng* _netmng, uint32_t _max_conn, bool _crypt = false, int _sleep_time = 1000 );
    void add_conn( const char* _ip, uint16_t _port, bool _add_tail = true );
    void run();
    void on_conn_disconnect( DPID _dpid, bool _reconnect );

protected:
    void clean();

    NetMng*         netmng_;            // parent netmng
    uint32_t        max_conn_;          // 最大可支持的连接数
    uint32_t        curr_conn_;         // 当前管理的连接数
    ConnInfo*       conn_infos_;        // 连接信息数组
    MyMap32*        connected_conns_;   // 已连接的map
    list_head       pending_unconnected_conns_; // 未连接并等待连接（IP和端口已赋值）的链表
    list_head       free_unconnected_conns_; // 未连接并空闲（IP和端口未赋值）的链表
    Mutex           mutex_;             // 互斥锁
    bool            crypt_;
    int             sleep_time_;
};

#define g_multi_connect_thread ( Singleton<MultiConnectThread>::instance_ptr() )

#endif //__CONNECT_THREAD_H__
