#include "curve_mng.h"
#include "log.h"
#include "lmisc.h"
#include "lua_utils.h"

using namespace Lua;

CurveMng::CurveMng()
{
    curve_map_.init( 32, 32, "curve_map_:curve_mng.cpp" );
}

CurveMng::~CurveMng()
{
    BezierCurve* curve = NULL;
    Node* node = curve_map_.first();
    while( node )
    {
        // 先转uintptr_t（指针大小的整数），再转指针，避免大小不匹配
        BezierCurve *curve = (BezierCurve*)(uintptr_t)(node->val);
        LOG(2)( "[CurveMng](~CurveMng) delete curve: 0x%x, id: %d", curve, node->key );
        delete curve;
        node = curve_map_.next( node );
    }
}

BezierCurve* CurveMng::get_curve( int _curve_id )
{
    BezierCurve* curve = NULL;
    curve_map_.find( _curve_id, (intptr_t&)curve );
    return curve;
}

bool CurveMng::create_curve( lua_State* _state, int _curve_id )
{
    BezierCurve* curve = NULL;
    if( curve_map_.find( _curve_id, (intptr_t&)curve ) )
    {
        LOG(2)( "[CurveMng](create_curve) update old curve: 0x%x, id: %d", curve, _curve_id );
        curve->clear();
    }
    else
    {
        curve = new BezierCurve();
        LOG(2)( "[CurveMng](create_curve) create new curve: 0x%x, id: %d", curve, _curve_id );
    }

    VECTOR3 pos;
    VECTOR3 handle1;
    VECTOR3 handle2;

    int point_count = lua_objlen( _state, -1 );
    for (int i = 1; i <= point_count; i++)
    {
        GET_TABLE( _state, -1, i, 3 )   // point i

        GET_TABLE( _state, -1, 1, 3 )   // point pos
        get_table_number_by_index( _state, -1, 1, pos.x );
        get_table_number_by_index( _state, -1, 2, pos.y );
        get_table_number_by_index( _state, -1, 3, pos.z );
        lua_pop( _state, 1 );

        GET_TABLE( _state, -1, 2, 3 )   // handle1 pos
        get_table_number_by_index( _state, -1, 1, handle1.x );
        get_table_number_by_index( _state, -1, 2, handle1.y );
        get_table_number_by_index( _state, -1, 3, handle1.z );
        lua_pop( _state, 1 );

        GET_TABLE( _state, -1, 3, 3 )   // handle2 pos
        get_table_number_by_index( _state, -1, 1, handle2.x );
        get_table_number_by_index( _state, -1, 2, handle2.y );
        get_table_number_by_index( _state, -1, 3, handle2.z );
        lua_pop( _state, 1 );

        curve->add_point( pos, handle1, handle2 );

        lua_pop( _state, 1 );
    }

    curve_map_.set( _curve_id, (intptr_t)curve );

    return true;
}

const char CurveMng::className[] = "CurveMng";
const bool CurveMng::gc_delete_ = false;

Lunar<CurveMng>::RegType CurveMng::methods[] =
{
	LUNAR_CURVEMNG_METHODS,
	{NULL, NULL}
};


int CurveMng::c_reload( lua_State* _state )
{
    lcheck_argc( _state, 1 );
    if( lua_type( _state, -1 ) != LUA_TTABLE )
    {
        lua_pushboolean( _state, false );
        return 1;
    }

    WHILE_TABLE(_state)
        int curve_id = (int)lua_tointeger( _state, -2 );
        if (!create_curve( _state, curve_id ))
        {
            lua_pop(_state, 2);
            lua_pushboolean( _state, false );
            lcheck_argc( _state, 2 );
            return 1;
        }
    END_WHILE(_state)

    lua_pushboolean( _state, true );
    lcheck_argc( _state, 2 );
    return 1;
}

