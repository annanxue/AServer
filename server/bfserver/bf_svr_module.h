#ifndef __BF_SVR_MODULE__
#define __BF_SVR_MODULE__

#include "app_base.h"
#include "netmng.h"
#include "msgtype.h"
#include "singleton.h"
#include "ar.h"
#include "lunar.h"

using namespace FF_Network;

class BFSvr : public NetMng
{
    public:
        BFSvr();
        ~BFSvr();
    public:
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
    public:
        void set_bf_num( int _num ) { bf_num_ = _num; }
        int  get_bf_num() { return bf_num_; }
        void set_mipush_tag( const char* _tag ) { strncpy( mipush_tag_, _tag, sizeof( mipush_tag_ ) ); }
    private:
        void (BFSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
    public:
        static const char className[];
        static Lunar<BFSvr>::RegType methods[];
        static const bool gc_delete_;
        char gamesvr_config_url_[APP_CFG_NAME_MAX];
        char platform_tag_[APP_CFG_NAME_MAX];
        char game_id_[APP_CFG_NAME_MAX];
        char mipush_tag_[APP_CFG_NAME_MAX];
    public:
        int c_get_bf_id( lua_State* _L );
        int c_get_bf_num( lua_State* _L );
	    int	c_get_gamesvr_config_url( lua_State* _L );
	    int	c_get_platform_tag( lua_State* _L );
	    int	c_get_mipush_tag( lua_State* _L );
	    int	c_get_game_id( lua_State* _L );
    private:
        int bf_num_;
};

class BFSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_bfsvr Singleton<BFSvr>::instance_ptr() 

#endif
