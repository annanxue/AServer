#include "db_client_module.h"
#include "game_lua_module.h"
#include "log.h"
#include <cstring>

namespace AServer {

DBClient::DBClient() {
    init_func_map();
}

DBClient::~DBClient() {
}

void DBClient::init_func_map() {
}

void DBClient::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
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

void DBClient::on_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("DBClient connected, dpid: %u", dpid);
    connected_ = true;
    send_register_server_id();
}

void DBClient::on_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("DBClient disconnected, dpid: %u", dpid);
    connected_ = false;
}

void DBClient::send_exec_sql(const char* sql) {
    if (connected_ && sock_map_.size() > 0) {
        send_msg(sql, strlen(sql), 1);
    }
}

void DBClient::lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid) {
    if (!L) return;

    lua_getglobal(L, "dbclient");
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
        ERR("DBClient Lua msg handle error: %s", err ? err : "unknown");
        lua_pop(L, 1);
    }
}

void DBClient::send_last_msg() {
    INFO("DBClient sending last message");
}

void DBClient::send_register_server_id() {
    INFO("DBClient registering server id");
}

bool DBClientModule::app_class_init() {
    INFO("DBClientModule initializing...");
    return true;
}

bool DBClientModule::app_class_destroy() {
    INFO("DBClientModule destroying...");
    return true;
}

}
