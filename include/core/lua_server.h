#ifndef ASERVER_LUA_SERVER_H
#define ASERVER_LUA_SERVER_H

#include "define.h"
#include "thread.h"
#include "singleton.h"
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <string>
#include <vector>
#include <map>

namespace AServer {

class LuaServer : public Singleton<LuaServer> {
public:
    LuaServer();
    virtual ~LuaServer();

    bool start_server(const std::string& luadir, const std::string& main_file);
    void check_gc(int32_t frame_left_time);
    bool do_file(const std::string& file);
    bool reload();
    int reload(const std::string& lua_file);
    int lua_mem_size();

    lua_State* L() { return L_; }

    virtual void register_lua_refs() {}
    virtual void register_class() {}
    virtual void register_global();
    virtual int32_t get_timer_ref() { return 0; }

    void unregister_lua_refs();
    bool get_table_field_ref(const std::string& table, const std::string& field, int32_t ref, int32_t index);
    int get_lua_ref(int ref);
    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

    static int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
    static int lua_pcall_ex(lua_State* L, int nargs, int nresults, int errfunc);

protected:
    lua_State* L_ = nullptr;
    std::vector<int> refs_;
    std::string luadir_;
    std::string main_file_;
    Mutex mutex_;

    static int traceback(lua_State* L);

public:
    static LuaServer* instance_ptr() {
        return Singleton<LuaServer>::instance_ptr();
    }
};

#define g_luasvr AServer::LuaServer::instance_ptr()

}

#endif
