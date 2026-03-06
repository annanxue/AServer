#include "game_svr_module.h"
#include "game_lua_module.h"
#include "log.h"
#include <cstring>

namespace AServer {

GameSvr::GameSvr() {
    init_func_map();

    max_con_ = 0;
    memset(ip_, 0, sizeof(ip_));
    port_ = 0;
    crypt_ = 1;
    poll_wait_time_ = 100;
    proc_thread_num_ = 4;
    allow_shops_on_bf_ = false;
    allow_wild_cross_zone_ = false;
    memset(gamesvr_config_url_, 0, sizeof(gamesvr_config_url_));
    memset(game_area_, 0, sizeof(game_area_));
    memset(platform_tag_, 0, sizeof(platform_tag_));
    memset(order_query_url_, 0, sizeof(order_query_url_));
    memset(order_ack_url_, 0, sizeof(order_ack_url_));
    memset(order_compensate_url_, 0, sizeof(order_compensate_url_));
    memset(game_id_, 0, sizeof(game_id_));
}

GameSvr::~GameSvr() {
}

void GameSvr::init_func_map() {
}

void GameSvr::set_start_param(int32_t max_con, const char* ip, int32_t port, int32_t crypt,
    int32_t poll_wait_time, int32_t proc_thread_num) {
    max_con_ = max_con;
    strncpy(ip_, ip, sizeof(ip_) - 1);
    port_ = port;
    crypt_ = crypt;
    poll_wait_time_ = poll_wait_time;
    proc_thread_num_ = proc_thread_num;
}

void GameSvr::on_client_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("Client connected, dpid: %u", dpid);
    if (g_luasvr) {
        lua_msg_handle(g_luasvr->L(), buf, buf_size, PT_CONNECT, dpid);
    }
}

void GameSvr::on_client_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("Client disconnected, dpid: %u", dpid);
    if (g_luasvr) {
        lua_msg_handle(g_luasvr->L(), buf, buf_size, PT_DISCONNECT, dpid);
    }
    del_sock(dpid);
}

void GameSvr::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
    if (packet_type == PT_CONNECT) {
        on_client_connect(dpid, msg, size);
    } else if (packet_type == PT_DISCONNECT) {
        on_client_disconnect(dpid, msg, size);
    } else {
        if (g_luasvr) {
            lua_msg_handle(g_luasvr->L(), msg, size, packet_type, dpid);
        }
    }
}

void GameSvr::lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid) {
    if (!L) return;

    lua_getglobal(L, "gamesvr");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "on_message");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    lua_pushlstring(L, msg, size);
    lua_pushinteger(L, size);
    lua_pushinteger(L, packet_type);
    lua_pushinteger(L, dpid);

    if (LuaServer::debug_call(L, 4, 0, 0) != 0) {
        const char* err = lua_tostring(L, -1);
        ERR("Lua msg handle error: %s", err ? err : "unknown");
        lua_pop(L, 1);
    }
}

void GameSvr::kick_player(DPID dpid) {
    INFO("Kicking player, dpid: %u", dpid);
    del_sock(dpid);
}

void GameSvr::kick_all_player() {
    INFO("Kicking all players");
    std::lock_guard<Mutex> lock(sock_mutex_);
    std::vector<DPID> to_remove;
    for (auto& pair : sock_map_) {
        CommonSocket* cs = pair.second;
        if (!cs->is_server()) {
            to_remove.push_back(pair.first);
        }
    }
    for (DPID dpid : to_remove) {
        del_sock(dpid);
    }
}

void GameSvr::on_server_pre_stop() {
    INFO("Server pre stop");
}

void GameSvr::set_gamesvr_config_url(const char* url) {
    strncpy(gamesvr_config_url_, url, sizeof(gamesvr_config_url_) - 1);
}

void GameSvr::set_game_area(const char* area) {
    strncpy(game_area_, area, sizeof(game_area_) - 1);
}

void GameSvr::set_is_china(bool is_china) {
    is_china_ = is_china;
}

void GameSvr::set_platform_tag(const char* tag) {
    strncpy(platform_tag_, tag, sizeof(platform_tag_) - 1);
}

void GameSvr::set_order_query_url(const char* url) {
    strncpy(order_query_url_, url, sizeof(order_query_url_) - 1);
}

void GameSvr::set_order_ack_url(const char* url) {
    strncpy(order_ack_url_, url, sizeof(order_ack_url_) - 1);
}

void GameSvr::set_order_compensate_url(const char* url) {
    strncpy(order_compensate_url_, url, sizeof(order_compensate_url_) - 1);
}

void GameSvr::set_mipush_tag(const char* tag) {
    strncpy(mipush_tag_, tag, sizeof(mipush_tag_) - 1);
}

void GameSvr::set_game_id(const char* id) {
    strncpy(game_id_, id, sizeof(game_id_) - 1);
}

bool GameSvrModule::app_class_init() {
    INFO("GameSvrModule initializing...");
    return true;
}

bool GameSvrModule::app_class_destroy() {
    INFO("GameSvrModule destroying...");
    return true;
}

}
