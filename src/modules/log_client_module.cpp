#include "log_client_module.h"
#include "game_lua_module.h"
#include "log.h"

namespace AServer {

LogClient::LogClient() {
    init_func_map();
}

LogClient::~LogClient() {
}

void LogClient::init_func_map() {
}

void LogClient::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
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

void LogClient::on_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("LogClient connected, dpid: %u", dpid);
}

void LogClient::on_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("LogClient disconnected, dpid: %u", dpid);
}

void LogClient::lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid) {
    if (!L) return;

    lua_getglobal(L, "logclient");
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
        ERR("LogClient Lua msg handle error: %s", err ? err : "unknown");
        lua_pop(L, 1);
    }
}

bool LogClientModule::app_class_init() {
    INFO("LogClientModule initializing...");
    return true;
}

bool LogClientModule::app_class_destroy() {
    INFO("LogClientModule destroying...");
    return true;
}

}
