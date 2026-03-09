#ifndef __LUA_SERVER_H__
#define __LUA_SERVER_H__

extern "C"
{
#include <lua.h>         
#include <lauxlib.h>
#include <lualib.h>
}

#include <stdint.h>
#include "autolock.h"

#ifdef DEBUG             
#include "lua_debugger.h"
#endif

const size_t LUASERVER_NAME_MAX = 1024;
const int32_t LUASERVER_REF_MAX = 1024;

class LuaServer
{
public:
    LuaServer();
    virtual     ~LuaServer();
    bool        start_server( const char* _luadir, const char* _main_file );
    void        check_gc( int32_t _frame_left_time );
	bool		do_file( const char* _file );

    lua_State* L() { return L_; } 

public:
    virtual void        register_lua_refs() {}
    virtual void        register_class() {}
    virtual void        register_global();
    virtual int32_t     get_timer_ref(){ return 0; }
	bool				reload();
    int                 reload( const char* _lua_file );
    int                 lua_mem_size();

public:
    void        unregister_lua_refs();
    bool        get_table_field_ref( const char* _table, const char* _field, int32_t _ref, int32_t _index );
    int         get_lua_ref( int _ref ); 
    void        lock() { mutex_.Lock(); }
    void        unlock() { mutex_.Unlock(); }

protected:
    lua_State*  L_;
    int32_t     refs_[LUASERVER_REF_MAX];
    char        luadir_[LUASERVER_NAME_MAX];
    char        main_file_[LUASERVER_NAME_MAX];

public:
    static int  debug_call( lua_State * _L, int32_t _nargs, int32_t _nresults, int32_t _errfunc );

private:
    Mutex       mutex_;
public:
#ifdef DEBUG             
    GScriptDebug* debugger_;
#endif
};

#endif
