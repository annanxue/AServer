#include "gm_svr_module.h"
#include "game_lua_module.h"
#include "log.h"

namespace AServer {

GmSvr::GmSvr() {
    init_func_map();
}

GmSvr::~GmSvr() {
}

void GmSvr::init_func_map() {
}

void GmSvr::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
    switch (packet_type) {
        case PT_CONNECT:
            on_client_connect(dpid, msg, size);
            break;
        case PT_DISCONNECT:
            on_client_disconnect(dpid, msg, size);
            break;
        default:
            if (g_luasvr) {
                lua_msg_handle(g_luasvr->L(), msg, size, packet_type, dpid);
            }
            break;
    }
}

void GmSvr::on_client_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("GM client connected, dpid: %u", dpid);
}

void GmSvr::on_client_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("GM client disconnected, dpid: %u", dpid);
    del_sock(dpid);
}

void GmSvr::lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid) {
    if (!L) return;

    lua_getglobal(L, "gmsvr");
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
        ERR("GmSvr Lua msg handle error: %s", err ? err : "unknown");
        lua_pop(L, 1);
    }
}

bool GmSvrModule::app_class_init() {
    INFO("GmSvrModule initializing...");
    return true;
}

bool GmSvrModule::app_class_destroy() {
    INFO("GmSvrModule destroying...");
    return true;
}

}
