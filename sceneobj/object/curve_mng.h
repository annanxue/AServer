#ifndef __CURVE_MNG_H__
#define __CURVE_MNG_H__

#include "singleton.h"
#include "lua_server.h"
#include "lunar.h"
#include "3dmath.h"
#include "mymap32.h"
#include "bezier_curve.h"

#define GET_TABLE( _state, _index, _n, _len )       \
{                                                   \
    lua_rawgeti( _state, _index, _n );              \
    if (lua_objlen( _state, -1 ) != _len)           \
    {                                               \
        lua_pop( _state, 1 );                       \
        return false;                               \
    }                                               \
}                                                   \

class CurveMng
{
    public:
        CurveMng();
        ~CurveMng();

    public:
        BezierCurve* get_curve( int _curve_id );

    private:
        bool create_curve( lua_State* _state, int _curve_id );

    private:
        MyMap32 curve_map_;

    public:
	    static const char className[];
	    static Lunar<CurveMng>::RegType methods[];
	    static const bool gc_delete_;

    public:
        int c_reload( lua_State* _state );
};

#define g_curvemng ( Singleton<CurveMng>::instance_ptr() )

#define LUNAR_CURVEMNG_METHODS	\
    method( CurveMng, c_reload ) \

#endif

