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
        virtual void msg_handle( const char* _msg, int _size, int _packet_type, DPID _dpid );
        void set_start_param(int32_t _max_con, char* _ip, int32_t _port, int32_t _crypt, int32_t _poll_wait_time, int32_t _proc_thread_num);
        void set_allow_shops_on_bf(bool _allow) { allow_shops_on_bf_ = _allow; }
        void set_allow_wild_cross_zone(bool _allow) { allow_wild_cross_zone_ = _allow; }
        void kick_player(DPID dpid);
        void kick_all_player();
        void on_server_pre_stop();
        void lua_set_max_user_num( lua_State* _L, int _max_user_num );
        void set_gamesvr_config_url( const char* _url ) { strncpy( gamesvr_config_url_, _url, sizeof( gamesvr_config_url_) ); }
        void set_game_area( const char* _game_area ) { strncpy( game_area_, _game_area, sizeof( game_area_) ); }
		void set_is_china(const bool _is_china) { is_china_ = _is_china; }
        void set_platform_tag( const char* _platform_tag ) { strncpy( platform_tag_, _platform_tag, sizeof( platform_tag_ ) ); }
        void set_order_query_url( const char* _url ) { strncpy( order_query_url_, _url, sizeof( order_query_url_ ) ); }
        void set_order_ack_url( const char* _url ) { strncpy( order_ack_url_, _url, sizeof( order_ack_url_ ) ); }
        void set_order_compensate_url( const char* _url ) { strncpy( order_compensate_url_, _url, sizeof( order_compensate_url_ ) ); }
		void set_ema_gift_order_query_url(const char* _url) { strncpy(ema_gift_order_query_url_, _url, sizeof(ema_gift_order_query_url_)); }
		void set_ema_gift_order_url_ack(const char* _url) { strncpy(ema_gift_order_url_ack_, _url, sizeof(ema_gift_order_url_ack_)); }
        void set_mipush_tag( const char* _tag ) { strncpy( mipush_tag_, _tag, sizeof( mipush_tag_ ) ); }
        void set_game_id( const char* _game_id) { strncpy( game_id_, _game_id, sizeof( game_id_) ); }

    private:
        void init_func_map();
        void on_client_connect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void on_client_disconnect( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );
        void lua_msg_handle( lua_State* _L, const char* _msg, int _size, int _packet_type, DPID _dpid );
    private:
        void (GameSvr::*func_map_[PT_PACKET_MAX])( Ar& _ar, DPID _dpid, const char* _buf, u_long _buf_size );

        int32_t max_con_;
        char ip_[APP_CFG_NAME_MAX];
        int32_t port_;
        int32_t crypt_;
        int32_t poll_wait_time_;
        int32_t proc_thread_num_;

		bool    is_china_;

        bool allow_shops_on_bf_;
        bool allow_wild_cross_zone_;
        char gamesvr_config_url_[APP_CFG_NAME_MAX];
        char game_area_[APP_CFG_NAME_MAX];
        char platform_tag_[APP_CFG_NAME_MAX];

        char order_query_url_[APP_CFG_NAME_MAX];
        char order_ack_url_[APP_CFG_NAME_MAX];
        char order_compensate_url_[APP_CFG_NAME_MAX];
		char ema_gift_order_query_url_[APP_CFG_NAME_MAX];
		char ema_gift_order_url_ack_[APP_CFG_NAME_MAX];
        char mipush_tag_[APP_CFG_NAME_MAX];
        char game_id_[APP_CFG_NAME_MAX];

    public:
        static const char className[];
        static Lunar<GameSvr>::RegType methods[];
        static const bool gc_delete_;

        int32_t c_get_server_id( lua_State* _L );
        int32_t c_get_server_ip( lua_State* _L );
        int32_t c_get_server_port( lua_State* _L );
        int32_t c_get_allow_shops_on_bf( lua_State* _L );
        int32_t c_get_allow_wild_cross_zone( lua_State* _L );
        int32_t c_get_user_num( lua_State* _L );
        int32_t c_kick_player( lua_State* _L );
        int32_t	c_start_server( lua_State* _L );
        int32_t	c_get_player_ip( lua_State* _L );
	    int32_t	c_get_gamesvr_config_url( lua_State* _L );
	    int32_t	c_get_game_area( lua_State* _L );
		int32_t	c_is_china(lua_State* _L);
	    int32_t	c_get_platform_tag( lua_State* _L );
	    int32_t	c_get_order_query_url( lua_State* _L );
	    int32_t	c_get_order_ack_url( lua_State* _L );
	    int32_t	c_get_order_compensate_url( lua_State* _L );
		int32_t	c_get_ema_gift_order_query_url(lua_State* _L);
		int32_t	c_get_ema_gift_order_url_ack(lua_State* _L);
	    int32_t	c_get_mipush_tag( lua_State* _L );
	    int32_t	c_get_game_id( lua_State* _L );
};

class GameSvrModule : public AppClassInterface
{
    public:
        bool app_class_init();
        bool app_class_destroy();
};

#define g_gamesvr Singleton<GameSvr>::instance_ptr() 

#endif
