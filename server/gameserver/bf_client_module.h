
#ifndef __BF_CLIENT_H__
#define __BF_CLIENT_H__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"
#include "define.h"

using namespace FF_Network;

class BFClient;

struct BFInfo
{
    char    ip_[APP_CFG_NAME_MAX];
    bool    connected_;
    int     port_;
    int     load_;
    DPID    dpid_;
    int     heart_beat_time_;
};

class ConnectBFThread : public Thread
{
public:
    void run();
    void init( BFClient* _bf_client, int _sleep, int _wait, int _crypt );
private:
    void send_heart_beat( DPID _dpid );
    void send_register( int _bf_id, DPID _dpid );
    void update_connect( BFInfo& _info, DPID _dpid );
private:
    BFClient* bf_client_;
    int sleep_;
    int crypt_;
};

class BFClient : public NetMng
{
public:
	BFClient();
	~BFClient();
public:
    void        msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
    void        on_heart_beat_reply( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    void        on_db_save_player( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    void        on_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    void        on_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    void        on_save_player( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
public:
    void        start_connect( int _sleep_time, int _wait_time, int _crypt );
    void        stop_connect();
    BFInfo*     get_bf_info() { return bf_info_; }
    int         get_bf_num() { return bf_num_; }
    void        set_bf_num( int _num ) { bf_num_ = _num; bf_info_ = new BFInfo[bf_num_]; }
private:
    int         choose_bfid_min_load();
    int         choose_bfid_random();
	int			choose_bfid_by_serverid(); 
    DPID        choose_dpid_min_load();
    bool        is_bf_ready();
public:
	static const char className[];
	static Lunar<BFClient>::RegType methods[];
	static const bool gc_delete_;
public:
    int         c_get_bf_ip( lua_State* _L );
    int         c_get_bf_port( lua_State* _L );
    int         c_get_is_connect( lua_State* _L );
    int         c_get_dpid_by_bfid( lua_State* _L );
    int         c_get_all_bf_dpid( lua_State* _L );
    int         c_get_center_dpid( lua_State* _L );
    int         c_get_all_dpid( lua_State* _L );
    int         c_choose_bf( lua_State* _L );
    int         c_get_random_bf( lua_State* _L );
    int         c_show_bfclient( lua_State* _L );
    int         c_get_bf_num( lua_State* _L );
	int         c_get_bf_by_serverid(lua_State* _L);
private:
    int         bf_num_;
    BFInfo*     bf_info_;      
    ConnectBFThread     connect_thread_;
private:
    void        init_func_map();
    void        (BFClient::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
	void		lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
};

class BFClientModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_bfclient Singleton<BFClient>::instance_ptr() 

#endif
