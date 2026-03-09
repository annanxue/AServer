#ifndef __MAGIC_AREA_H__
#define __MAGIC_AREA_H__

#include "ctrl.h"

class MagicArea : public Ctrl
{
public:
	MagicArea();
public:
    virtual void        process_parallel();
    virtual void        process_serial();
    virtual void        safe_delete(); 

private:
    bool                do_delete_;
    int                 frame_max_;
    int                 frame_count_;
    float               drag_speed_;
    OBJID               owner_id_;
    OBJID               target_id_;
    bool                frame_search_;
    int                 last_drag_frame_;

private:
    void                do_drag();
    void                do_frame_search();

public:
	static const char className[];
	static Lunar<MagicArea>::RegType methods[];
	static const bool gc_delete_;

public:
    int                 c_get_do_delete( lua_State* _state );
    int                 c_set_do_delete( lua_State* _state );
    int                 c_get_owner_id( lua_State* _state );
    int                 c_set_owner_id( lua_State* _state );
    int                 c_get_target_id( lua_State* _state );
    int                 c_set_target_id( lua_State* _state );
    int                 c_set_frame_max( lua_State* _state );
    int                 c_get_drag_speed( lua_State* _state );
    int                 c_set_drag_speed( lua_State* _state );
    int                 c_set_frame_search( lua_State* _state );
};

#define LUNAR_MAGIC_AREA_METHODS                            \
	method( MagicArea, c_get_do_delete ),			        \
	method( MagicArea, c_set_do_delete ),			        \
	method( MagicArea, c_get_owner_id ),			        \
	method( MagicArea, c_set_owner_id ),			        \
	method( MagicArea, c_get_target_id ),			        \
	method( MagicArea, c_set_target_id ),			        \
	method( MagicArea, c_set_frame_max ),			        \
	method( MagicArea, c_get_drag_speed ),			        \
	method( MagicArea, c_set_drag_speed ),			        \
	method( MagicArea, c_set_frame_search )

#endif

