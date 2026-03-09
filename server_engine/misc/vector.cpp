#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include "vector.h"

int vc3_init( VECTOR3& _dest, float _x, float _y, float _z )
{
	_dest.x = _x;
	_dest.y = _y;
	_dest.z = _z;
	return 0;
}
int vc3_copy( VECTOR3& _dest, const VECTOR3& _src1 )
{
	_dest.x = _src1.x;
	_dest.y = _src1.y;
	_dest.z = _src1.z;
	return 0;
}

int vc3_add( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2 )
{
	_dest.x = _src1.x + _src2.x;
	_dest.y = _src1.y + _src2.y;
	_dest.z = _src1.z + _src2.z;
	return 0;
}

int vc3_sub( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2 )
{
	_dest.x = _src1.x - _src2.x;
	_dest.y = _src1.y - _src2.y;
	_dest.z = _src1.z - _src2.z;
	return 0;
}

int vc3_mul( VECTOR3& _dest, const VECTOR3& _src1, float _src2 )
{
	_dest.x = _src1.x * _src2;
	_dest.y = _src1.y * _src2;
	_dest.z = _src1.z * _src2;
	return 0;
}

int vc3_norm( VECTOR3& _dest, const VECTOR3& _src1 , float _len)
{
	float lenq = sqrt( _src1.x * _src1.x + _src1.y * _src1.y + _src1.z * _src1.z );
	if ( lenq <= 0.0f ) return -1;
	lenq = _len / lenq;
	_dest.x = _src1.x * lenq;
	_dest.y = _src1.y * lenq;
	_dest.z = _src1.z * lenq;
	return 0;
}


int vc3_cross( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2 )
{
	VECTOR3 ret;
	ret.x = _src1.y * _src2.z - _src1.z * _src2.y;
	ret.y = _src1.z * _src2.x - _src1.x * _src2.z;
	ret.z = _src1.x * _src2.y - _src1.y * _src2.x;

	_dest.x = ret.x;
	_dest.y = ret.y;
	_dest.z = ret.z;
	return 0;
}

float vc3_dot( const VECTOR3& _src1, const VECTOR3& _src2 )
{
	return _src1.x * _src2.x + _src1.y * _src2.y + _src1.z * _src2.z;
}

float vc3_len( const VECTOR3& _src1 )
{
	return ( _src1.x * _src1.x + _src1.y * _src1.y + _src1.z * _src1.z );
}

float vc3_xzlen( const VECTOR3& _src1 )
{
	return ( _src1.x * _src1.x + _src1.z * _src1.z );
}

float vc3_lensq( const VECTOR3& _src1 )
{
	return ( sqrt( _src1.x * _src1.x + _src1.y * _src1.y + _src1.z * _src1.z ) );
}


float vc3_xzlensq( const VECTOR3& _src1 )
{
	return ( sqrt( _src1.x * _src1.x + _src1.z * _src1.z ) );
}

int vc3_equal( const VECTOR3& _src1, const VECTOR3& _src2 )
{
	return ( fabs( _src1.x - _src2.x ) <= 0.00001f 
			&& fabs( _src1.y - _src2.y ) <= 0.00001f 
			&& fabs( _src1.z - _src2.z ) <= 0.00001f )
			? 1 : 0;
}


int vc3_print( const VECTOR3& _src1 )
{
	fprintf( stderr, "{ %f, %f, %f }\n", _src1.x, _src1.y, _src1.z );
	return 0;
}

int vc3_intrp( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2, float _rate )
{
	_dest.x = _src1.x + (_src2.x - _src1.x) * _rate;
	_dest.y = _src1.y + (_src2.y - _src1.y) * _rate;
	_dest.z = _src1.z + (_src2.z - _src1.z) * _rate;
	return 0;
}

int vc3_iszero( const VECTOR3& _src1 )
{
	if (_src1.x > FLT_EPSILON || _src1.x < -FLT_EPSILON)
	{
		return 0;
	}
	if (_src1.y > FLT_EPSILON || _src1.y < -FLT_EPSILON)
	{
		return 0;
	}
	if (_src1.z > FLT_EPSILON || _src1.z < -FLT_EPSILON)
	{
		return 0;
	}
	return 1;
}


#ifdef __cplusplus

VECTOR3 operator+( const VECTOR3& _src1, const VECTOR3& _src2 )
{
	VECTOR3 _dest;
	vc3_add( _dest, _src1, _src2 );
	return _dest;
}


VECTOR3 operator-( const VECTOR3& _src1, const VECTOR3& _src2 )
{
	VECTOR3 _dest;
	vc3_sub( _dest, _src1, _src2 );
	return _dest;
}


VECTOR3 operator*( const VECTOR3& _src1, float _src2 )
{
	VECTOR3 _dest;
	vc3_mul( _dest, _src1, _src2 );
	return _dest;
}


int operator==( const VECTOR3& _src1, const VECTOR3& _src2 )
{
	return vc3_equal( _src1, _src2 );
}

int operator!=( const VECTOR3& _src1, const VECTOR3& _src2 )
{
	return !vc3_equal( _src1, _src2 );
}


#endif

int vc4_init( VECTOR4& _dest, float _x, float _y, float _z, float _w )
{
	_dest.x = _x;
	_dest.y = _y;
	_dest.z = _z;
	_dest.w = _w;
	return 0;
}
int vc4_copy( VECTOR4& _dest, const VECTOR4& _src1 )
{
	_dest.x = _src1.x;
	_dest.y = _src1.y;
	_dest.z = _src1.z;
	_dest.w = _src1.w;
	return 0;
}

int vc4_add( VECTOR4& _dest, const VECTOR4& _src1, const VECTOR4& _src2 )
{
	_dest.x = _src1.x + _src2.x;
	_dest.y = _src1.y + _src2.y;
	_dest.z = _src1.z + _src2.z;
	_dest.w = _src1.w + _src2.w;
	return 0;
}

int vc4_sub( VECTOR4& _dest, const VECTOR4& _src1, const VECTOR4& _src2 )
{
	_dest.x = _src1.x - _src2.x;
	_dest.y = _src1.y - _src2.y;
	_dest.z = _src1.z - _src2.z;
	_dest.w = _src1.w - _src2.w;
	return 0;
}

int vc4_mul( VECTOR4& _dest, const VECTOR4& _src1, float _src2 )
{
	_dest.x = _src1.x * _src2;
	_dest.y = _src1.y * _src2;
	_dest.z = _src1.z * _src2;
	_dest.w = _src1.w * _src2;
	return 0;
}

int vc4_norm( VECTOR4& _dest, const VECTOR4& _src1 )
{
	float lenq = sqrt( _src1.x * _src1.x + _src1.y * _src1.y + _src1.z * _src1.z + _src1.w * _src1.w );
	if ( lenq <= 0.0f ) return -1;
	lenq = 1.0f / lenq;
	_dest.x *= _src1.x * lenq;
	_dest.y *= _src1.y * lenq;
	_dest.z *= _src1.z * lenq;
	_dest.w *= _src1.w * lenq;
	return 0;
}

int vc4_cross( VECTOR4& _dest, const VECTOR4& _src1, const VECTOR4& _src2 )
{
	VECTOR3 ret;
	ret.x = _src1.y * _src2.z - _src1.z * _src2.y;
	ret.y = _src1.z * _src2.x - _src1.x * _src2.z;
	ret.z = _src1.x * _src2.y - _src1.y * _src2.x;

	_dest.x = ret.x;
	_dest.y = ret.y;
	_dest.z = ret.z;
	_dest.w = 0.0f;
	return 0;
}

int vc4_dot( float& _dest, const VECTOR4& _src1, const VECTOR4& _src2 )
{
	_dest = _src1.x * _src2.x + _src1.y * _src2.y + _src1.z * _src2.z + _src1.w * _src2.w;
	return 0;
}


float vc4_len( const VECTOR4& _src1 )
{
	return ( _src1.x * _src1.x + _src1.y * _src1.y + _src1.z * _src1.z );
}

float vc4_lensq( const VECTOR4& _src1 )
{
	return ( sqrt( _src1.x * _src1.x + _src1.y * _src1.y + _src1.z * _src1.z + _src1.w * _src1.w ) );
}

int vc4_print( const VECTOR4& _src1 )
{
	fprintf( stderr, "{ %f, %f, %f, %f }\n", _src1.x, _src1.y, _src1.z, _src1.w );
	return 0;
}
