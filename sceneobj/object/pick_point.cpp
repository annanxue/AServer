#include "pick_point.h"
#include "define_func.h"

PickPoint::PickPoint()
{
	settype( OT_PICK_POINT );
}

PickPoint::~PickPoint()
{
}

/************************************************************
 * for lua
 ***********************************************************/
 
const char PickPoint::className[] = "PickPoint";
const bool PickPoint::gc_delete_ = false;

Lunar<PickPoint>::RegType PickPoint::methods[] =
{	
	LUNAR_OBJ_METHODS,
	LUNAR_CTRL_METHODS,
	LUNAR_PICK_POINT_METHODS,
	{NULL, NULL}
};


int PickPoint::c_test_func( lua_State* _state )
{
	lcheck_argc( _state, 0 );
	return 0;
}
