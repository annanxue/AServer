#ifndef __BEZIER_CURVE_H__
#define __BEZIER_CURVE_H__

#include "3dmath.h"
#include "misc.h"
#include "define.h"

class BezierPoint
{
public:
    BezierPoint( const VECTOR3& _pos, const VECTOR3& _handle1, const VECTOR3& _handle2 );

public:
    bool                has_handle1() { return handle1_.x != pos_.x || handle1_.y != pos_.y || handle1_.z != pos_.z; }
    bool                has_handle2() { return handle2_.x != pos_.x || handle2_.y != pos_.y || handle2_.z != pos_.z; }

public:
    list_head           link_;
    VECTOR3             pos_;
    VECTOR3             handle1_;
    VECTOR3             handle2_;
};

class BezierCurve
{
private: 
    list_head           points_;
    bool                is_closed_;
    float               length_;
    int                 resolution_;
    bool                is_dirty_;

public:
	BezierCurve();
	~BezierCurve();

public:
    void                add_point( const VECTOR3& _pos, const VECTOR3& _handle1, const VECTOR3& _handle2 );
    VECTOR3             get_pos( float _t );
    VECTOR3             get_tangent( float _t );
    float               get_length();
    void                set_closed( bool _is_closed );
    void                set_dirty( bool _is_dirty );
    void                clear();
    float               find_section( BezierPoint** _p1, BezierPoint** _p2, float _t );

public:
    static VECTOR3      get_pos( BezierPoint* _p1, BezierPoint* _p2, float _t );
    static VECTOR3      get_cubic_pos( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, const VECTOR3 _v4, float _t );
    static VECTOR3      get_quadratic_pos( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, float _t );
    static VECTOR3      get_linear_pos( const VECTOR3& _v1, const VECTOR3& _v2, float _t );
    static VECTOR3      get_tangent( BezierPoint* _p1, BezierPoint* _p2, float _t );
    static VECTOR3      get_cubic_tangent( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, const VECTOR3 _v4, float _t );
    static VECTOR3      get_quadratic_tangent( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, float _t );
    static VECTOR3      get_linear_tangent( const VECTOR3& _v1, const VECTOR3& _v2, float _t );
    static float        get_approximate_length( BezierPoint* _p1, BezierPoint* _p2, int _resolution = 10 );
};

#endif
