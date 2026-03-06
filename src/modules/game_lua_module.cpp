#include "game_lua_module.h"
#include "game_svr_module.h"
#include "log.h"

namespace AServer {

void GameLuaSvr::register_lua_refs() {
    LuaServer::register_lua_refs();

    refs_.resize(MAX_LUA_REF, LUA_NOREF);

    lua_getglobal(L(), "require");
    if (lua_isfunction(L(), -1)) {
        if (lua_pcall(L(), 0, 1, 0) == 0) {
            INFO("Lua refs registered");
        }
    }
    lua_settop(L(), 0);
}

void GameLuaSvr::register_class() {
    LuaServer::register_class();

    INFO("GameLuaSvr class registered");
}

void GameLuaSvr::register_global() {
    LuaServer::register_global();
}

void GameLuaSvr::on_exit() {
    INFO("GameLuaSvr on_exit");
}

bool LuaModule::app_class_init() {
    INFO("LuaModule initializing...");

    if (!g_luasvr) {
        ERR("LuaServer not initialized");
        return false;
    }

    std::string luadir = "./lua";
    std::string main_file = "main.lua";

    if (!g_luasvr->start_server(luadir, main_file)) {
        ERR("Failed to start Lua server");
        return false;
    }

    INFO("LuaModule initialized successfully");
    return true;
}

bool LuaModule::app_class_destroy() {
    INFO("LuaModule destroying...");

    if (g_luasvr) {
        g_luasvr->unregister_lua_refs();
    }

    return true;
}

}
