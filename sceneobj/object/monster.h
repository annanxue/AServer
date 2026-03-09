#ifndef __MONSTER_H__
#define __MONSTER_H__

#include "spirit.h"
#include "component_ai.h"

using namespace SGame;

class Monster : public Spirit
{
public:
	Monster();
	virtual ~Monster();
public:
	virtual void		process_serial();
	virtual int			view_add( Ctrl* _obj );
	virtual int			view_remove( Ctrl* _obj );
protected:
	void				on_view_add( Ctrl* _obj );
	void				on_view_remove( Ctrl* _obj );
public:
	int c_do_set_angle_y( lua_State* _state );
	int c_replace_pos( lua_State* _state );
	int c_set_npc_type( lua_State* _state );

public:
	static const char className[];
	static Lunar<Monster>::RegType methods[];
	static const bool gc_delete_;
	void set_ai(AI *_ai){ ai_ = _ai; }
	AI* get_ai(){return ai_;}
private:
	AI *ai_;
    int npc_type_;
};


#define LUNAR_MONSTER_METHODS				\
	method( Monster, c_replace_pos ),			\
	method( Monster, c_set_npc_type ),			\
	method( Monster, c_do_set_angle_y )


#endif //__Monster_H__

