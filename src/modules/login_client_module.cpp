#include "login_client_module.h"
#include "game_lua_module.h"
#include "log.h"

namespace AServer {

LoginClient::LoginClient() {
    init_func_map();
}

LoginClient::~LoginClient() {
}

void LoginClient::init_func_map() {
}

void LoginClient::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
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

void LoginClient::on_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("LoginClient connected, dpid: %u", dpid);
    send_register_server_id();
}

void LoginClient::on_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("LoginClient disconnected, dpid: %u", dpid);
}

void LoginClient::send_exec_sql(const char* sql) {
}

void LoginClient::lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid) {
    if (!L) return;

    lua_getglobal(L, "loginclient");
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
        ERR("LoginClient Lua msg handle error: %s", err ? err : "unknown");
        lua_pop(L, 1);
    }
}

void LoginClient::send_register_server_id() {
    INFO("LoginClient registering server id");
}

bool LoginClientModule::app_class_init() {
    INFO("LoginClientModule initializing...");
    return true;
}

bool LoginClientModule::app_class_destroy() {
    INFO("LoginClientModule destroying...");
    return true;
}

}
