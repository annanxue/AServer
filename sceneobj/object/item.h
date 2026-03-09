
#ifndef _ITEM_H_
#define _ITEM_H_

#include "ctrl.h"

class Item: public Ctrl
{
public:
	Item();
	virtual ~Item();
    virtual void process_serial();
	void		player_enter( OBJID _player_ctrl_id );
public:
	static const char className[];
	static Lunar<Item>::RegType methods[];
	static const bool gc_delete_;

public:
	int	c_test_func( lua_State* _state );

};

#define LUNAR_ITEM_METHODS			\
	method( Item, c_test_func )     
#endif
