#include "lua_server.h"
#include "log.h"
#include <iostream>
#include <sstream>

namespace AServer {

LuaServer::LuaServer() {
}

LuaServer::~LuaServer() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool LuaServer::start_server(const std::string& luadir, const std::string& main_file) {
    luadir_ = luadir;
    main_file_ = main_file;

    L_ = luaL_newstate();
    if (!L_) {
        ERR("Failed to create Lua state");
        return false;
    }

    luaL_openlibs(L_);

    INFO("Lua server started, dir: %s, main: %s", luadir_.c_str(), main_file_.c_str());

    refs_.resize(64, LUA_NOREF);

    register_class();
    register_lua_refs();
    register_global();

    std::string main_path = luadir_ + "/" + main_file_;
    if (!do_file(main_path)) {
        ERR("Failed to load main file: %s", main_path.c_str());
        return false;
    }

    INFO("Lua server initialized successfully");
    return true;
}

void LuaServer::check_gc(int32_t frame_left_time) {
    if (!L_) return;

    static int gc_count = 0;
    gc_count++;

    if (gc_count % 100 == 0) {
        lua_gc(L_, LUA_GCCOLLECT, 0);
    }
}

bool LuaServer::do_file(const std::string& file) {
    if (!L_) return false;

    int ret = luaL_dofile(L_, file.c_str());
    if (ret != 0) {
        const char* err = lua_tostring(L_, -1);
        ERR("Lua dofile error: %s", err ? err : "unknown");
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

bool LuaServer::reload() {
    if (!L_) return false;

    INFO("Reloading Lua scripts...");

    lua_gc(L_, LUA_GCCOLLECT, 0);

    unregister_lua_refs();

    if (!do_file(luadir_ + "/" + main_file_)) {
        ERR("Failed to reload main file");
        return false;
    }

    INFO("Lua scripts reloaded successfully");
    return true;
}

int LuaServer::reload(const std::string& lua_file) {
    std::string full_path = luadir_ + "/" + lua_file;
    return do_file(full_path) ? 0 : -1;
}

int LuaServer::lua_mem_size() {
    if (!L_) return 0;
    return static_cast<int>(lua_gc(L_, LUA_GCCOUNT, 0) * 1024 + lua_gc(L_, LUA_GCCOUNTB, 0));
}

void LuaServer::register_global() {
}

void LuaServer::unregister_lua_refs() {
    if (!L_) return;

    for (auto ref : refs_) {
        if (ref != LUA_NOREF && ref != LUA_REFNIL) {
            luaL_unref(L_, LUA_REGISTRYINDEX, ref);
        }
    }
}

bool LuaServer::get_table_field_ref(const std::string& table, const std::string& field, int32_t ref, int32_t index) {
    if (!L_) return false;

    lua_getglobal(L_, table.c_str());
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }

    lua_getfield(L_, -1, field.c_str());
    refs_[ref] = luaL_ref(L_, -2);
    lua_pop(L_, 1);

    return true;
}

int LuaServer::get_lua_ref(int ref) {
    if (!L_ || ref < 0 || ref >= (int)refs_.size()) {
        return 0;
    }

    int lua_ref = refs_[ref];
    if (lua_ref == LUA_NOREF || lua_ref == LUA_REFNIL) {
        lua_pushnil(L_);
    } else {
        lua_rawgeti(L_, LUA_REGISTRYINDEX, lua_ref);
    }
    return lua_ref;
}

int LuaServer::traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (msg) {
        lua_pushstring(L, msg);
    } else if (!lua_isnoneornil(L, 1)) {
        if (!luaL_callmeta(L, 1, "__tostring")) {
            lua_pushliteral(L, "(no error message)");
        }
    }
    return 1;
}

int LuaServer::debug_call(lua_State* L, int nargs, int nresults, int errfunc) {
    int base = lua_gettop(L) - nargs - 1;
    lua_pushcfunction(L, traceback);
    lua_insert(L, base);
    int ret = lua_pcall(L, nargs, nresults, base);
    lua_remove(L, base);
    return ret;
}

}
