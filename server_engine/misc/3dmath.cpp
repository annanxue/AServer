#include <assert.h>
#include "3dmath.h"
#include <string.h>

float vc3_get_angle( const VECTOR3& src1 )
{
	return atan2f( src1.x, -src1.z );
}

float vc3_get_angle_x( const VECTOR3& src1 )
{
	float xz = sqrt( src1.x * src1.x + src1.z * src1.z );
	return ( fabs( xz ) < 0.00001f ) ? -( src1.y/fabs( src1.y ) )*DX_PI/2.0f : -atanf( src1.y / xz );
}

int mx_3d_scaling( MATRIX& dest, float sx, float sy, float sz )
{
	mx_initn( dest, sx,   0.0f, 0.0f, 0.0f,
					0.0f, sy,   0.0f, 0.0f,
					0.0f, 0.0f, sz,   0.0f,
					0.0f, 0.0f, 0.0f, 1.0f
	);
	return 0;
}

int mx_3d_rotation_x( MATRIX& dest, float rx )
{
	mx_initn( dest, 1.0f, 0.0f,     0.0f,     0.0f,
					0.0f, cos(rx),  sin(rx),  0.0f,
					0.0f, -sin(rx), cos(rx),  0.0f,
					0.0f, 0.0f,     0.0f,     1.0f
	);
	return 0;
}

int mx_3d_rotation_y( MATRIX& dest, float ry )
{
	mx_initn( dest, cos(ry),  0.0f, -sin(ry), 0.0f,
					0.0f,     1.0f, 0.0f,     0.0f,
					sin(ry),  0.0f, cos(ry),  0.0f,
					0.0f,     0.0f, 0.0f,     1.0f
	);
	return 0;
}

int mx_3d_rotation_z( MATRIX& dest, float rz )
{
	mx_initn( dest,  cos(rz), sin(rz), 0.0f, 0.0f,
					-sin(rz), cos(rz), 0.0f, 0.0f,
					0.0f,     0.0f,    1.0f, 0.0f,
					0.0f,     0.0f,    0.0f, 1.0f
	);
	return 0;
}

int mx_3d_translation( MATRIX& dest, float tx, float ty, float tz )
{
	mx_initn( dest, 1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					tx,   ty,   tz,   1.0f
	);
	return 0;
}

float get_degree( VECTOR3 _dest_pos, VECTOR3 _src_pos )
{
	_src_pos.y = _dest_pos.y = 0.0f;
	VECTOR3 v1 = velocity_to_vec( 0.0f, 1.0f );
	VECTOR3 v2;
	vc3_sub( v2, _dest_pos , _src_pos );

	vc3_norm( v1, v1 );
	vc3_norm( v2, v2 );

	float dot = vc3_dot( v1, v2 );
	if( dot > 1.0f ) dot = 1.0f;
	else if( dot < -1.0f ) dot = -1.0f;

	float radian = acos( dot );
	float degree = d3d_to_degree( radian );
	if( v2.x < 0.0f )
		degree = 360.0f - degree;
	else
		degree = degree;

	return degree;
}


float get_degree_x( const VECTOR3& _dest_pos, const VECTOR3& _src_pos )
{
	float	dist = _dest_pos.y - _src_pos.y;
	VECTOR3 v_xz;
	vc3_sub( v_xz, _dest_pos , _src_pos );
	v_xz.y = 0.0f;							/*!<  只需xz平面上的bactor */
	float	len_xz = vc3_lensq( v_xz );		/*!<  XZ平面上的长度.*/
	float	z_dist = len_xz;

	float r = (float)atan2( dist, -z_dist );
	float a = d3d_to_degree( r );
	a = -a;

	if( a > 90.0f )
		a	= 180.0f - a;
	else if( a < -90.0f )
		a	= -180.0f - a;
	return a;
}


#ifdef __cplusplus
VECTOR3 velocity_to_vec( float _angle, float _velocity )
{
	float theta = d3d_to_radian( _angle );
	float fx = sin( theta ) * _velocity;
	float fz = -cos( theta ) * _velocity;
	return VECTOR3( fx, 0.0f, fz );
} 
#endif

#define MAX_DEGREE 360000
VECTOR3 t_math[ MAX_DEGREE ];

void init_t_math()
{
    memset( t_math, 0, sizeof( t_math ) );
    double d = M_PI * 2 / MAX_DEGREE;
    int i = 0;
    for( i=0; i<MAX_DEGREE; ++i ) {
        t_math[i].x = (float)sin( i * d );
        t_math[i].z = -(float)cos( i * d );
    }
}


float SIN( float d )
{
    int i = (int)(d * 1000);
    while( i < 0 ) {
        i += MAX_DEGREE;
    }
    while( i >= MAX_DEGREE ) {
        i -= MAX_DEGREE;
    }
    return t_math[i].x;
}

float COS( float d )
{
    int i = (int)(d * 1000);
    while( i < 0 ) {
        i += MAX_DEGREE;
    }
    while( i >= MAX_DEGREE ) {
        i -= MAX_DEGREE;
    }
    return -t_math[i].z;
}

VECTOR3& GET_DIR( float d )
{
    int i = (int)(d * 1000);
    while( i < 0 ) {
        i += MAX_DEGREE;
    }
    while( i >= MAX_DEGREE ) {
        i -= MAX_DEGREE;
    }
    return t_math[i];
}

bool vc3_check_out_range( VECTOR3& _src, VECTOR3 _dest, float _range )
{
    VECTOR3 dist = _dest - _src;
    if( fabsf( dist.x ) <= _range && fabsf( dist.z ) <= _range ) { 
        return  ( (dist.x * dist.x + dist.z * dist.z) >= ( _range * _range  ) );
    }   
    return true;
}

