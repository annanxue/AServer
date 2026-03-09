#include "lua_utils.h"
#include "ffsys.h"

#include <string.h>

/************************************************************************
 *                          Lua
************************************************************************/

void Lua::get_table_string( lua_State* _L, int32_t _table_index, const char* _key, char* _dst_str, size_t _dst_len )
{
    lua_pushvalue( _L, _table_index );
    lua_pushstring( _L, _key );
    lua_gettable( _L, -2 );
    if( !lua_isnil( _L, -1 ) ) {
        strlcpy( _dst_str, lua_tostring( _L, -1 ), _dst_len );
    }
    lua_pop( _L, 2 );
}

void Lua::get_table_string_by_index( lua_State* _L, int32_t _table_index, int32_t _index, char* _dst_str, size_t _dst_len )
{
    lua_pushvalue( _L, _table_index );
    lua_rawgeti( _L, -1, _index );
    if( !lua_isnil( _L, -1 ) ) {
        strlcpy( _dst_str, lua_tostring( _L, -1 ), _dst_len );
    }
    lua_pop( _L, 2 );
}

int32_t Lua::get_table( lua_State* _L, int32_t _table_index, const char* _key )
{
    lua_pushvalue( _L, _table_index );    
    lua_pushstring( _L, _key );
    lua_gettable( _L, -2 );

    if( lua_isnil( _L, -1 ) || !lua_istable( _L, -1 ) )
    {
        lua_pop( _L, 2 );
        return -1;
    }
    else
    {
        lua_insert( _L, -2 );
        lua_pop( _L, 1 );
        return lua_gettop( _L );
    }

    return -1;
}

int32_t Lua::get_table( lua_State* _L, int32_t _table_index, int32_t _index )
{
    lua_pushvalue( _L, _table_index );
    lua_pushnumber( _L, _index );
    lua_gettable( _L, -2 );

    if( lua_isnil( _L, -1 ) || !lua_istable( _L, -1 ) )
    {
        lua_pop( _L, 2 );
        return -1;
    }
    else
    {
        lua_insert( _L, -2 );
        lua_pop( _L, 1 );
        return lua_gettop( _L );
    }

    return -1;
}


int32_t Lua::do_file( lua_State* _L, const char* _file_name )
{
    if( luaL_dofile( _L, _file_name ) ) {
        return 1;
    }

    return 0;
}

