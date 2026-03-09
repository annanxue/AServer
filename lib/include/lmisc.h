#ifndef __LMISC_H__
#define __LMISC_H__

#include "log.h"
#include <stdint.h>

extern "C"
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#define llua_fail( _L, _file, _line ) \
{                                     \
    ERR(2)( "[LUAWRAPPER](lmisc) %s:%d lua_call failed\n\t%s", _file, _line, lua_tostring( _L, -1 ) ); \
    lua_pop( _L, 1 ); \
}

#define lcheck_argc( _L, _argc )\
{                                   \
    assert( _L );                   \
    int32_t argc = lua_gettop( _L );\
    if( argc != _argc ){            \
        ERR(2)( "[LUAWRAPPER](lmisc) %s:%d argc %d, expect %d", __FILE__, __LINE__, argc, _argc ); \
        return 0;                   \
    }                               \
}
#define llua_call( _L, _nargs, _nresults, _errfunc ) \
    ( LuaServer::debug_call( _L, _nargs, _nresults, _errfunc ) )

#endif
