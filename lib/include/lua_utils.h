#ifndef __LUA_UTILS_H__
#define __LUA_UTILS_H__

extern "C"
{
#include <lua.h>         
#include <lauxlib.h>
#include <lualib.h>
}

#include <stdint.h>

namespace Lua 
{        
    template <typename T>
        inline void get_table_number( lua_State* _L, int32_t _table_index, const char* _key, T& _dst_number );
	template <typename T>
		inline void get_table_split_number(lua_State* _L, int32_t _table_index, const char* _key, T& _dst_number , int remain_bits );
    template <typename T>
        inline void get_table_number_by_index( lua_State* _L, int32_t _table_index, int32_t _index, T& _dst_number );
    void get_table_string( lua_State* _L, int32_t _table_index, const char* _key, char* _dst_str, size_t _dst_len );
    void get_table_string_by_index( lua_State* _L, int32_t _table_index, int32_t _index, char* _dst_str, size_t _dst_len );
    int32_t get_table( lua_State* _L, int32_t _table_index, const char* _key );
    int32_t get_table( lua_State* _L, int32_t _table_index, int32_t _index );

	int32_t do_file( lua_State* _L, const char* _file_name );
};

#define WHILE_TABLE(L) lua_pushnil(L); while( lua_next(L, -2) ) {
#define END_WHILE(L) lua_pop(L, 1); }

template <typename T>
inline void Lua::get_table_number( lua_State* _L, int32_t _table_index, const char* _key, T& _dst_number )
{
    lua_pushvalue( _L, _table_index );
    lua_pushstring( _L, _key );
    lua_gettable( _L, -2 );
    if( !lua_isnil( _L, -1 ) ) {
        _dst_number = (T)lua_tonumber( _L, -1 );
    }   
    lua_pop( _L, 2 );
}

// remain_bits : 表示保留后几位
template <typename T>
inline void Lua::get_table_split_number(lua_State* _L, int32_t _table_index, const char* _key, T& _dst_number  , int remain_bits )
{
	if (remain_bits <= 0)
		return; 
	int64_t number = 0;
	get_table_number(_L, _table_index, _key, number);
	int64_t temp = number;
	int64_t needless = 1;
	int count = 0;
	while (temp && count != remain_bits){
		temp /= 10; 
		count++;
		needless *= 10;
	}
	temp *= needless; 
	_dst_number = number - temp;
}


template <typename T>
inline void Lua::get_table_number_by_index( lua_State* _L, int32_t _table_index, int32_t _index, T& _dst_number )
{
    lua_pushvalue( _L, _table_index );
    lua_rawgeti( _L, -1, _index );
    if( !lua_isnil( _L, -1 ) ) { 
        _dst_number = (T)lua_tonumber( _L, -1 );
    }   
    lua_pop( _L, 2 );
}

#endif
