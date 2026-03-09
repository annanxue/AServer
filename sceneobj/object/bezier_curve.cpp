#include "bezier_curve.h"
#include "log.h"
#include "3dmath.h"
#include "lmisc.h"

BezierPoint::BezierPoint( const VECTOR3& _pos, const VECTOR3& _handle1, const VECTOR3& _handle2 )
{
    pos_ = _pos;
    handle1_ = _handle1;
    handle2_ = _handle2;
}

BezierCurve::BezierCurve()
{
    INIT_LIST_HEAD( &points_ );
    resolution_  = 30;
    is_dirty_    = true;
    is_closed_   = false;
    length_      = 0;
}

BezierCurve::~BezierCurve()
{
    clear();
}

void BezierCurve::clear()
{
    while (!list_empty( &points_ ))
    {
        list_head* pos = points_.next;
        list_del( pos );
        BezierPoint* p = list_entry( pos, BezierPoint, link_ );
        LOG(2)( "[BezierCurve](clear) curve: 0x%x, point: 0x%x", this, p );
        delete p;
    }
}

void BezierCurve::add_point( const VECTOR3& _pos, const VECTOR3& _handle1, const VECTOR3& _handle2 )
{
    BezierPoint* p = new BezierPoint( _pos, _handle1, _handle2 );
    list_add_tail( &(p->link_), &points_ );
    set_dirty( true );
    LOG(2)( "[BezierCurve](add_point) curve: 0x%x, point: 0x%x, pos: {%f,%f,%f}, handle1: {%f,%f,%f}, handle2: {%f,%f,%f}", 
            this, p, _pos.x, _pos.y, _pos.z, _handle1.x, _handle1.y, _handle1.z, _handle2.x, _handle2.y, _handle2.z ); 
}

float BezierCurve::find_section( BezierPoint** _p1, BezierPoint** _p2, float _t )
{
    if (list_empty( &points_ ) || points_.next->next == &points_)
    {
        ERR(2)( "[BezierCurve](find_section) bezier point count < 2" );
        return -1;
    }

    if (_t <= 0)
    {
        _t = 0;
    }
    else if (_t >= 1)
    {
        _t = 1;
    }

    float total_percent = 0;
    float curve_percent = 0;
    float total_length = get_length();

    BezierPoint* cur_p1 = NULL;
    BezierPoint* cur_p2 = NULL;

    list_head* pos;
    list_for_each( pos, &(points_) )
    {
        if (pos->next == &points_)
        {
            break;
        }
        cur_p1 = list_entry( pos, BezierPoint, link_ );
        cur_p2 = list_entry( pos->next, BezierPoint, link_ );
        curve_percent = get_approximate_length( cur_p1, cur_p2, resolution_ ) / total_length;
        if (total_percent + curve_percent > _t)
        {
            *_p1 = cur_p1;
            *_p2 = cur_p2;
            return (_t - total_percent) / curve_percent;
        }
        total_percent += curve_percent;
    }

    if (is_closed_)
    {
        *_p1 = list_entry( points_.prev, BezierPoint, link_ );
        *_p2 = list_entry( points_.next, BezierPoint, link_ );
        return (_t - total_percent) / curve_percent;
    }
    else
    {
        *_p1 = list_entry( points_.prev->prev, BezierPoint, link_ );
        *_p2 = list_entry( points_.prev, BezierPoint, link_ );
        return 1;
    }
}

VECTOR3 BezierCurve::get_pos( float _t )
{
    BezierPoint* p1;
    BezierPoint* p2;
    float t = find_section( &p1, &p2, _t );
    if ( t > 0 )
    {
        return get_pos( p1, p2, t );
    }
    return VECTOR3(0, 0, 0);
}

VECTOR3 BezierCurve::get_tangent( float _t )
{
    BezierPoint* p1;
    BezierPoint* p2;
    float t = find_section( &p1, &p2, _t );
    if ( t > 0 )
    {
        return get_tangent( p1, p2, t );
    }
    return VECTOR3(0, 0, 0);
}

float BezierCurve::get_length()
{
    if (is_dirty_)
    {
        length_ = 0;
        BezierPoint* p1;
        BezierPoint* p2;
        list_head* pos;
        list_for_each( pos, &(points_) )
        {
            if (pos->next == &points_)
            {
                break;
            }
            p1 = list_entry( pos, BezierPoint, link_ );
            p2 = list_entry( pos->next, BezierPoint, link_ );
            length_ += get_approximate_length( p1, p2, resolution_ );
        }
        if (is_closed_ && !list_empty( &points_ ))
        {
            p1 = list_entry( points_.prev, BezierPoint, link_ );     // last point
            p2 = list_entry( points_.next, BezierPoint, link_ );     // first point
            length_ += get_approximate_length( p1, p2, resolution_ );
        }
        set_dirty( false );
    }
    return length_;
}

void BezierCurve::set_closed( bool _is_closed )
{
    if (is_closed_ == _is_closed)
    {
        return;
    }
    is_closed_ = _is_closed;
    set_dirty( true );
}

void BezierCurve::set_dirty( bool _is_dirty )
{
    is_dirty_ = _is_dirty;
}

VECTOR3 BezierCurve::get_pos( BezierPoint* _p1, BezierPoint* _p2, float _t )
{
    if (_p1->has_handle2())
    {
        if (_p2->has_handle1())
        {
            return get_cubic_pos( _p1->pos_, _p1->handle2_, _p2->handle1_, _p2->pos_, _t );
        }
        else
        {
            return get_quadratic_pos( _p1->pos_, _p1->handle2_, _p2->pos_, _t );
        }
    }
    else
    {
        if (_p2->has_handle1())
        {
            return get_quadratic_pos( _p1->pos_, _p2->handle1_, _p2->pos_, _t );
        }
        else
        {
            return get_linear_pos( _p1->pos_, _p2->pos_, _t );
        }
    }
}

VECTOR3 BezierCurve::get_tangent( BezierPoint* _p1, BezierPoint* _p2, float _t )
{
    if (_p1->has_handle2())
    {
        if (_p2->has_handle1())
        {
            return get_cubic_tangent( _p1->pos_, _p1->handle2_, _p2->handle1_, _p2->pos_, _t );
        }
        else
        {
            return get_quadratic_tangent( _p1->pos_, _p1->handle2_, _p2->pos_, _t );
        }
    }
    else
    {
        if (_p2->has_handle1())
        {
            return get_quadratic_tangent( _p1->pos_, _p2->handle1_, _p2->pos_, _t );
        }
        else
        {
            return get_linear_tangent( _p1->pos_, _p2->pos_, _t );
        }
    }
}

VECTOR3 BezierCurve::get_cubic_pos( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, const VECTOR3 _v4, float _t )
{
    VECTOR3 part1 = _v1 * pow( 1 - _t, 3 );
    VECTOR3 part2 = _v2 * ( 3 * pow( 1 - _t, 2 ) * _t );
    VECTOR3 part3 = _v3 * ( 3 * ( 1 - _t ) * pow( _t, 2 ) );
    VECTOR3 part4 = _v4 * pow( _t, 3 );
    return part1 + part2 + part3 + part4;
}

VECTOR3 BezierCurve::get_quadratic_pos( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, float _t )
{
    VECTOR3 part1 = _v1 * pow( 1 - _t, 2 );
    VECTOR3 part2 = _v2 * ( 2 * ( 1 - _t ) * _t );
    VECTOR3 part3 = _v3 * pow( _t, 2 );
    return part1 + part2 + part3;
}

VECTOR3 BezierCurve::get_linear_pos( const VECTOR3& _v1, const VECTOR3& _v2, float _t )
{
    return _v1 + ( ( _v2 - _v1 ) * _t );
}

VECTOR3 BezierCurve::get_cubic_tangent( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, const VECTOR3 _v4, float _t )
{
    VECTOR3 v1 = _v1 * (1 - _t) + _v2 * _t;
    VECTOR3 v2 = _v2 * (1 - _t) + _v3 * _t;
    VECTOR3 v3 = _v3 * (1 - _t) + _v4 * _t;
    v1 = v1 * (1 - _t) + v2 * _t;
    v2 = v2 * (1 - _t) + v3 * _t;
    return v2 - v1;
}

VECTOR3 BezierCurve::get_quadratic_tangent( const VECTOR3& _v1, const VECTOR3& _v2, const VECTOR3& _v3, float _t )
{
    VECTOR3 v1 = _v1 * (1 - _t) + _v2 * _t;
    VECTOR3 v2 = _v2 * (1 - _t) + _v3 * _t;
    return v2 - v1;
}

VECTOR3 BezierCurve::get_linear_tangent( const VECTOR3& _v1, const VECTOR3& _v2, float _t )
{
    return _v2 - _v1;
}

float BezierCurve::get_approximate_length( BezierPoint* _p1, BezierPoint* _p2, int _resolution )
{
    float res = _resolution;
    float total = 0;
    VECTOR3 last_pos = _p1->pos_;
    VECTOR3 cur_pos;

    for (int i = 0; i < _resolution + 1; i++)
    {
        cur_pos = get_pos( _p1, _p2, i / res);
        total += vc3_lensq( cur_pos - last_pos );
        last_pos = cur_pos;
    }
    return total;
}

