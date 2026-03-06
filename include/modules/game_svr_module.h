#ifndef ASERVER_GAME_SVR_MODULE_H
#define ASERVER_GAME_SVR_MODULE_H

#include "define.h"
#include "app_base.h"
#include "net.h"
#include "singleton.h"
#include "lua_server.h"
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace AServer {

class GameSvr : public NetMng {
public:
    GameSvr();
    virtual ~GameSvr();

    virtual void msg_handle(const char* _msg, int _size, int _packet_type, DPID _dpid) override;

    void set_start_param(int32_t max_con, const char* ip, int32_t port, int32_t crypt,
        int32_t poll_wait_time, int32_t proc_thread_num);
    void set_allow_shops_on_bf(bool allow) { allow_shops_on_bf_ = allow; }
    void set_allow_wild_cross_zone(bool allow) { allow_wild_cross_zone_ = allow; }
    void kick_player(DPID dpid);
    void kick_all_player();
    void on_server_pre_stop();
    void set_gamesvr_config_url(const char* url);
    void set_game_area(const char* area);
    void set_is_china(bool is_china);
    void set_platform_tag(const char* tag);
    void set_order_query_url(const char* url);
    void set_order_ack_url(const char* url);
    void set_order_compensate_url(const char* url);
    void set_mipush_tag(const char* tag);
    void set_game_id(const char* id);

private:
    void init_func_map();
    void on_client_connect(DPID dpid, const char* buf, int buf_size);
    void on_client_disconnect(DPID dpid, const char* buf, int buf_size);
    void lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid);

    int32_t max_con_ = 0;
    char ip_[128];
    int32_t port_ = 0;
    int32_t crypt_ = 1;
    int32_t poll_wait_time_ = 100;
    int32_t proc_thread_num_ = 4;

    bool is_china_ = false;
    bool allow_shops_on_bf_ = false;
    bool allow_wild_cross_zone_ = false;

    char gamesvr_config_url_[256];
    char game_area_[64];
    char platform_tag_[64];
    char order_query_url_[256];
    char order_ack_url_[256];
    char order_compensate_url_[256];
    char mipush_tag_[64];
    char game_id_[64];

public:
    static GameSvr* instance_ptr() {
        return Singleton<GameSvr>::instance_ptr();
    }
};

class GameSvrModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

#define g_gamesvr AServer::GameSvr::instance_ptr()

}

#endif
