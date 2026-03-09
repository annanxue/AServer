#ifndef __UTIL_FUNC_H__
#define __UTIL_FUNC_H__

#include <sys/types.h>

extern "C"
{
   #include <lua.h>
   #include <lauxlib.h>
   #include <lualib.h>
}

#include "log.h"

inline void GetTableNumber( lua_State* L, int nTableIndex, const char* szKey, short& nDstNumber )
{
	lua_pushstring( L, szKey );
	lua_gettable( L, nTableIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		nDstNumber = (short)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no key named: %s", szKey );
	lua_pop( L, 1 );
}


inline void GetTableNumber( lua_State* L, int nTableIndex, const char* szKey, int& nDstNumber )
{
	lua_pushstring( L, szKey );
	lua_gettable( L, nTableIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		nDstNumber = (int)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no key named: %s", szKey );
	lua_pop( L, 1 );
}


inline void GetTableNumber( lua_State* L, int nTableIndex, const char* szKey, float& nDstNumber )
{
	lua_pushstring( L, szKey );
	lua_gettable( L, nTableIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		nDstNumber = (float)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no key named: %s", szKey );
	lua_pop( L, 1 );
}


inline void GetTableNumber( lua_State* L, int nTableIndex, const char* szKey, ulong& nDstNumber )
{
	lua_pushstring( L, szKey );
	lua_gettable( L, nTableIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		nDstNumber = (ulong)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no key named: %s", szKey );
	lua_pop( L, 1 );
}


inline void GetTableNumber( lua_State* L, int nTableIndex, const char* szKey, long& nDstNumber )
{
	lua_pushstring( L, szKey );
	lua_gettable( L, nTableIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		nDstNumber = (long)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no key named: %s", szKey );
	lua_pop( L, 1 );
}


inline void GetTableString( lua_State* L, int nTableIndex, const char* szKey, char* szDstString )
{
	lua_pushstring( L, szKey );
	lua_gettable( L, nTableIndex );
	if( !lua_isnil(L, -1) && lua_isstring(L, -1) )
		strlcpy( szDstString, lua_tostring(L, -1), lua_strlen(L, -1) + 1 );
	else
		LOG(2)( "[TABLE](key) has no key named: %s", szKey );
	lua_pop( L, 1 );
}


inline void GetTableNumberByIndex( lua_State* L, int nTableIndex, int nIndex, int& nDstNumber )
{
	lua_rawgeti( L, nTableIndex, nIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		nDstNumber = (int)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no index: %s", nIndex );
	lua_pop( L, 1 );
}


inline void GetTableNumberByIndex( lua_State* L, int nTableIndex, int nIndex, float& fDstNumber )
{
	lua_rawgeti( L, nTableIndex, nIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		fDstNumber = (float)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no index: %s", nIndex );
	lua_pop( L, 1 );
}


inline void GetTableNumberByIndex( lua_State* L, int nTableIndex, int nIndex, ulong& dwDstNumber )
{
	lua_rawgeti( L, nTableIndex, nIndex );
	if( !lua_isnil(L, -1) && lua_isnumber(L, -1) )
		dwDstNumber = (ulong)lua_tonumber(L, -1);
	else
		LOG(2)( "[TABLE](key) has no index: %s", nIndex );
	lua_pop( L, 1 );
}


inline void GetTableStringByIndex( lua_State* L, int nTableIndex, int nIndex, char* szDstString )
{
	lua_rawgeti( L, nTableIndex, nIndex );
	if( !lua_isnil(L, -1) && lua_isstring(L, -1) )
		strlcpy( szDstString, lua_tostring(L, -1), lua_strlen(L, -1)+1 );
	else
		LOG(2)( "[TABLE](key) has no index: %s", nIndex );
	lua_pop( L, 1 );
}


#endif

