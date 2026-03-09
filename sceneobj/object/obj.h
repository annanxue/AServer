#ifndef __OBJ_H__
#define __OBJ_H__

#include "3dmath.h"
#include "misc.h"
#include "define.h"
#include "ar.h"
#include "lunar.h"

#define CHECK_NULL_POS( pos ) if( pos.x == 0 && pos.z == 0 ) ERR( 2 )( "[POSITION] set null pos at %s:%d", __FILE__, __LINE__ );

class World;

class Obj
{
public:
	Obj();
	virtual ~Obj();		//虚析构函数，使之可以被继承

	void				settype(int _type) { type_ |= _type; }
	int					gettype() const { return type_; }

    void                serialize_unchange_ar( Ar& _ar );
    void                serialize_sometimes_change_ar( Ar& _ar );
    void                serialize_always_change_ar( Ar& _ar, bool _active );          // 主角非主角的差别只能放在肯定会改变的部分

	/*! 对象类型按位判断*/
	int					is_type( int _type ) const { return (type_ & _type); }
	int					is_player() const { return (type_ & OT_PLAYER); }
	int					is_monster() const { return (type_ & OT_MONSTER); }
	int					is_npc() const { return (type_ & OT_NPC); }
	int					is_pet() const { return (type_ & OT_PET); }
	int					is_spirit() const { return (type_ & OT_SPRITE); }
	int					is_item() const { return (type_ & OT_ITEM); }
	int					is_common() const { return (type_ & OT_COMMON); }
	int					is_ctrl() const { return (type_ & OT_CTRL); }
	int					is_static() const { return (type_ & OT_STATIC); }
	int					is_ignore_search() const { return (type_ & (OT_MAGIC_AREA | OT_ITEM)); }
    int                 is_bullet() const { return (type_ & OT_BULLET); }
    int 				is_trigger() const { return (type_ & OT_TRIGGER); }
	int					is_robot() const { return (type_ & OT_ROBOT); }
    int					is_pick_point() const { return (type_ & OT_PICK_POINT); }
	int					is_missile() const { return (type_ & OT_MISSILE); }

#ifdef DEBUG
    // 序列化开销统计
	enum {
        S_COMMON = 1,
        S_ITEM,
        S_SHIP,
        S_MONSTER,
        S_PET,
        S_PLAYER,
        MAX_S_TYPE,
    };
    struct SerializeStat {
        unsigned long long time_;
        int count_;
    };
    static SerializeStat serialize_stats_[MAX_S_TYPE];
    static void print_serialize_stats();
#endif

	/*! 这里只是简单设置obj位置,涉及场景的操作是在Ctrl下完成*/
	void				set_index( int _index ) { index_ = _index; sometimes_change_ = true; }
	int					get_index() { return index_; }

	virtual int			setpos( VECTOR3& _pos );
    int                 setpos_y( float _y );
	VECTOR3				getpos() const { return pos_; }
	int					get_linktype() const;
	int					get_linklevel() const { return link_level_; }
	void				set_delete() { delete_ = TRUE; }
	int					is_delete() const { return delete_; }
	void				set_prev_node( Obj* _obj ) { prev_ = _obj; }
	Obj*				get_prev_node() const { return prev_; }
	void				set_next_node( Obj* _obj ) { next_ = _obj; }
	Obj*				get_next_node() const { return next_; }
	void				ins_next_node( Obj* _obj );
	void				del_node();
	int					load_model( const char* filename );
	void				set_height( float _height ) { height_ = _height; }
	float				get_height();
	void				set_radius( float _radius ) { radius_ = _radius; }
	float				get_radius(); 
	void				set_attack_radius( float _radius ) { attack_radius_ = _radius; }
	float				get_attack_radius(); 
    void                set_search_body( bool _search_body ) { search_body_ = _search_body; }
    bool                is_search_body() { return search_body_; }
    bool                is_range_obj_ground( Obj* _obj, float _range );

	enum { LinkStatic = 0, LinkPlayer = 1, LinkDynamic = 2, LinkTypeMax = 3 };

    void				set_angle_y( float _angle );
	float				get_angle_y() const			{ return angle_.y; }
    void				set_angle_x( float _angle );
	float				get_angle_x() const			{ return angle_.x; }
    void				set_angle_z( float _angle );
	float				get_angle_z() const			{ return angle_.z; }
    float               get_scale() {return scale_.x;}
	
    void				add_angle_y( float _angle );
    void				add_angle_x( float _angle );

    int                 x_;
    int                 z_;

    // 序列化相关
    bool                first_time_;
    bool                sometimes_change_;
    int                 sometimes_change_offset_;
    int                 always_change_offset_;
    int                 data_ver_;

private:
	VECTOR3				pos_;			/*! 当前位置*/
	VECTOR3				scale_;
	VECTOR3			    angle_;
	int					type_;			/*! 对象类型*/
	int					index_;								/*! 类型 */

	int					link_level_;	/*! linkmap所在层*/
	Obj*				prev_;			/*! linkmap链表前指针*/
	Obj*				next_;			/*! linkmap链表后指针*/
	int					delete_;        /*! 是否已从世界中删去*/ 

    float               height_;
    float               radius_;
    float               attack_radius_;
    bool                search_body_;

public:	
	int c_gettype( lua_State* _L );
	int c_settype( lua_State* _L );
	int c_istype( lua_State* _L );
	
	int c_getpos( lua_State* _L );
	int c_setpos( lua_State* _L );
	int c_is_delete( lua_State* _L );

	int c_get_index( lua_State* _L );
	int c_set_index( lua_State* _L );

	int c_get_angle( lua_State* _L );
	int c_set_angle( lua_State* _L );

	int c_get_scale( lua_State* _L );
	int c_set_scale( lua_State* _L );
	
	int c_get_angle_x( lua_State* _L );
	int c_get_angle_y( lua_State* _L );
	int c_get_angle_z( lua_State* _L );
	int c_set_angle_x( lua_State* _L );
	int c_set_angle_y( lua_State* _L );
	int c_set_angle_z( lua_State* _L );

	int c_set_height( lua_State* _L );
	int c_set_radius( lua_State* _L );
	int c_get_radius( lua_State* _L );
	int c_get_dest_angle( lua_State* _L );
    int c_get_this( lua_State* _L );
    int c_set_change( lua_State* _L );
    int c_set_attack_radius( lua_State* _L );
    int c_get_attack_radius( lua_State* _L );
    int c_set_search_body( lua_State* _L );
    int c_is_search_body( lua_State* _L );
    int c_get_angle_y_vec( lua_State* _state );

    static const char className[];
	static Lunar<Obj>::RegType methods[];
	static const bool gc_delete_;		
	
};


inline int is_valid_obj( Obj* _obj )
{
	return _obj && ( _obj->is_delete() == FALSE );
}

inline int is_invalid_obj( Obj* _obj )
{
	return !_obj || _obj->is_delete();
}

//! 只有在这里定义过的函数在lua脚本中才可用
#define LUNAR_OBJ_METHODS			\
	method( Obj, c_gettype ),		\
	method( Obj, c_istype ),		\
	method( Obj, c_settype ),		\
	method( Obj, c_getpos ),		\
	method( Obj, c_setpos ),		\
	method( Obj, c_get_index ),		\
	method( Obj, c_get_angle ),		\
	method( Obj, c_get_angle_x ),	\
	method( Obj, c_get_angle_y ),	\
	method( Obj, c_get_angle_z ),	\
	method( Obj, c_set_angle_x ),	\
	method( Obj, c_set_angle_y ),	\
	method( Obj, c_set_angle_z ),	\
	method( Obj, c_set_index ),		\
	method( Obj, c_set_angle ),		\
	method( Obj, c_is_delete ),		\
	method( Obj, c_get_dest_angle ),\
	method( Obj, c_set_scale ),		\
	method( Obj, c_get_scale ),		\
	method( Obj, c_get_this ),		\
	method( Obj, c_set_height ),    \
	method( Obj, c_set_radius ),    \
	method( Obj, c_get_radius ),    \
    method( Obj, c_set_change ),    \
    method( Obj, c_set_attack_radius ), \
    method( Obj, c_get_attack_radius ), \
    method( Obj, c_set_search_body ),   \
    method( Obj, c_is_search_body ),    \
    method( Obj, c_get_angle_y_vec)

#endif
