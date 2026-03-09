#ifndef _PICK_POINT_H_
#define _PICK_POINT_H_

#include "ctrl.h"

class PickPoint: public Ctrl
{
public:
	PickPoint();
	virtual ~PickPoint();
public:
	static const char className[];
	static Lunar<PickPoint>::RegType methods[];
	static const bool gc_delete_;

public:
	int	c_test_func( lua_State* _state );
};

#define LUNAR_PICK_POINT_METHODS			\
	method( PickPoint, c_test_func )     
#endif
