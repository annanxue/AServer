#include "gm_client_module.h"
#include "game_lua_module.h"
#include "log.h"

namespace AServer {

GMClient::GMClient() {
    init_func_map();
}

GMClient::~GMClient() {
}

void GMClient::init_func_map() {
}

void GMClient::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
    switch (packet_type) {
        case PT_CONNECT:
            on_connect(dpid, msg, size);
            break;
        case PT_DISCONNECT:
            on_disconnect(dpid, msg, size);
            break;
        default:
            if (g_luasvr) {
                lua_msg_handle(g_luasvr->L(), msg, size, packet_type, dpid);
            }
            break;
    }
}

void GMClient::on_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("GMClient connected, dpid: %u", dpid);
}

void GMClient::on_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("GMClient disconnected, dpid: %u", dpid);
}

void GMClient::lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid) {
    if (!L) return;

    lua_getglobal(L, "gmclient");
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

    if (LuaServer::lua_pcall_ex(L, 3, 0, 0) != 0) {
        const char* err = lua_tostring(L, -1);
        ERR("GMClient Lua msg handle error: %s", err ? err : "unknown");
        lua_pop(L, 1);
    }
}

bool GMClientModule::app_class_init() {
    INFO("GMClientModule initializing...");
    return true;
}

bool GMClientModule::app_class_destroy() {
    INFO("GMClientModule destroying...");
    return true;
}

}
