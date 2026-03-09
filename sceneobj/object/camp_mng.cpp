#include "camp_mng.h"
#include "log.h"
#include "lmisc.h"

const int DEFAULT_CAMP_RELATION = -1;

const int CAMP_RELATION_FRIEND = 0;
const int CAMP_RELATION_ENEMY = 1;

CampMng::CampMng()
{
    memset( relations_, DEFAULT_CAMP_RELATION, sizeof(relations_) );
}

// camp:[1,N]
int CampMng::get_relation( int _camp1, int _camp2 )
{
    if( _camp1 > 0 && _camp1 < CAMP_TABLE_LEN_MAX && _camp2 > 0 && _camp2 < CAMP_TABLE_LEN_MAX )
    {
        return relations_[_camp1][_camp2];
    }
    return DEFAULT_CAMP_RELATION;
}

bool CampMng::is_friend( Spirit* _spirit1, Spirit* _spirit2 )
{
    int camp1 = _spirit1->get_camp();
    int camp2 = _spirit2->get_camp();
    int relation = get_relation( camp1, camp2 );
    return ((relation == CAMP_RELATION_FRIEND) || (_spirit1 == _spirit2));
}

bool CampMng::is_enemy( Spirit* _spirit1, Spirit* _spirit2 )
{
    int camp1 = _spirit1->get_camp();
    int camp2 = _spirit2->get_camp();
    int relation = get_relation( camp1, camp2 );
    return ((relation == CAMP_RELATION_ENEMY) && (_spirit1 != _spirit2));
}

const char CampMng::className[] = "CampMng";
const bool CampMng::gc_delete_ = false;

Lunar<CampMng>::RegType CampMng::methods[] =
{
	LUNAR_CAMPMNG_METHODS,
	{NULL, NULL}
};


int CampMng::c_read_cfg( lua_State* _state )
{
	lcheck_argc( _state, 1 );
    // CAMPS
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        //ERR(2)("[CampMng]load CAMPS.lua: return is not table");
        lua_pushboolean( _state, false );
        return 1;
    }
    int len1 = lua_objlen( _state, -1 );
    for(int i = 1; i <= len1; i++ )
    {
        lua_rawgeti( _state, -1, i );                       // CAMPS camp_map
        int len2 = lua_objlen( _state, -1 );
        for(int j = 1; j <= len2; j++ )
        {
            lua_rawgeti( _state, -1, j );                   // CAMPS camp_map relation
            relations_[i][j] = lua_tointeger( _state, -1 );
            lua_pop( _state, 1 );                           // CAMPS camp_map
        }
        lua_pop( _state, 1 );                               // CAMPS
    }
    lua_pushboolean( _state, true );
    return 1;
}

int CampMng::c_get_relation( lua_State* _state )
{
	lcheck_argc( _state, 2 );
    int camp1 = lua_tointeger( _state, 1 );
    int camp2 = lua_tointeger( _state, 2 );
    lua_pushinteger( _state, get_relation( camp1, camp2 ) );
    return 1;
}

