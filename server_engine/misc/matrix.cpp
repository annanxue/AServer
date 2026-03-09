#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "matrix.h"

//#define USE_ASM


#define DET3(m,a1,a2,a3,b1,b2,b3) (\
						m._##a1##b1 * m._##a2##b2 * m._##a3##b3 + m._##a1##b2 * m._##a2##b3 * m._##a3##b1 + m._##a1##b3 * m._##a2##b1 * m._##a3##b2 - \
						m._##a1##b1 * m._##a2##b3 * m._##a3##b2 - m._##a1##b2 * m._##a2##b1 * m._##a3##b3 - m._##a1##b3 * m._##a2##b2 * m._##a3##b1 \
						)

int mx_init( MATRIX& _dest, float* _src1 )
{
	if( !_src1 ) return -1;
	_dest._11 = _src1[0];
	_dest._12 = _src1[1];
	_dest._13 = _src1[2];
	_dest._14 = _src1[3];
	
	_dest._21 = _src1[4];
	_dest._22 = _src1[5];
	_dest._23 = _src1[6];
	_dest._24 = _src1[7];
	
	_dest._31 = _src1[8];
	_dest._32 = _src1[9];
	_dest._33 = _src1[10];
	_dest._34 = _src1[11];
	
	_dest._41 = _src1[12];
	_dest._42 = _src1[13];
	_dest._43 = _src1[14];
	_dest._44 = _src1[15];
	
	return 0;
}

int mx_initn( MATRIX& _dest, float _a11, float _a12, float _a13, float _a14,
							float _a21, float _a22, float _a23, float _a24, 
							float _a31, float _a32, float _a33, float _a34, 
							float _a41, float _a42, float _a43, float _a44)
{
	_dest._11 = _a11 ;
    _dest._12 = _a12 ;
    _dest._13 = _a13 ;
    _dest._14 = _a14 ;
                
    _dest._21 = _a21 ;
    _dest._22 = _a22 ;
    _dest._23 = _a23 ;
    _dest._24 = _a24 ;

    _dest._31 = _a31 ;
    _dest._32 = _a32 ;
    _dest._33 = _a33 ;
    _dest._34 = _a34 ;

    _dest._41 = _a41 ;
    _dest._42 = _a42 ;
    _dest._43 = _a43 ;
    _dest._44 = _a44 ;
	
	return 0;
}   
		
    
int mx_identity( MATRIX& _dest )
{   
    mx_initn( _dest, 1.0f, 0.0f, 0.0f, 0.0f,
    				0.0f, 1.0f, 0.0f, 0.0f,
    				0.0f, 0.0f, 1.0f, 0.0f,
    				0.0f, 0.0f, 0.0f, 1.0f );
	return 0;
}   
    
int mx_copy( MATRIX& _dest, const MATRIX& _src1 )
{
	_dest._11 = _src1._11;
	_dest._12 = _src1._12;
	_dest._13 = _src1._13;
	_dest._14 = _src1._14;

	_dest._21 = _src1._21;
	_dest._22 = _src1._22;
	_dest._23 = _src1._23;
	_dest._24 = _src1._24;

	_dest._31 = _src1._31;
	_dest._32 = _src1._32;
	_dest._33 = _src1._33;
	_dest._34 = _src1._34;

	_dest._41 = _src1._41;
	_dest._42 = _src1._42;
	_dest._43 = _src1._43;
	_dest._44 = _src1._44;

	return 0;
}

int mx_add( MATRIX& _dest, const MATRIX& _src1, const MATRIX& _src2 )
{
	_dest._11 = _src1._11 + _src2._11;
	_dest._12 = _src1._12 + _src2._12;
	_dest._13 = _src1._13 + _src2._13;
	_dest._14 = _src1._14 + _src2._14;

	_dest._21 = _src1._21 + _src2._21;
	_dest._22 = _src1._22 + _src2._22;
	_dest._23 = _src1._23 + _src2._23;
	_dest._24 = _src1._24 + _src2._24;

	_dest._31 = _src1._31 + _src2._31;
	_dest._32 = _src1._32 + _src2._32;
	_dest._33 = _src1._33 + _src2._33;
	_dest._34 = _src1._34 + _src2._34;

	_dest._41 = _src1._41 + _src2._41;
	_dest._42 = _src1._42 + _src2._42;
	_dest._43 = _src1._43 + _src2._43;
	_dest._44 = _src1._44 + _src2._44;
	return 0;
}

int mx_sub( MATRIX& _dest, const MATRIX& _src1, const MATRIX& _src2 )
{
	_dest._11 = _src1._11 - _src2._11;
	_dest._12 = _src1._12 - _src2._12;
	_dest._13 = _src1._13 - _src2._13;
	_dest._14 = _src1._14 - _src2._14;

	_dest._21 = _src1._21 - _src2._21;
	_dest._22 = _src1._22 - _src2._22;
	_dest._23 = _src1._23 - _src2._23;
	_dest._24 = _src1._24 - _src2._24;

	_dest._31 = _src1._31 - _src2._31;
	_dest._32 = _src1._32 - _src2._32;
	_dest._33 = _src1._33 - _src2._33;
	_dest._34 = _src1._34 - _src2._34;

	_dest._41 = _src1._41 - _src2._41;
	_dest._42 = _src1._42 - _src2._42;
	_dest._43 = _src1._43 - _src2._43;
	_dest._44 = _src1._44 - _src2._44;
	return 0;
}

int mx_mulf( MATRIX& _dest, const MATRIX& _src1, const float _src2 )
{
#ifdef USE_ASM
	VECTOR4 v;
	v.m[0] = v.m[1] = v.m[2] =v.m[3] = 2.0f;
    __asm__ __volatile__(
    "leal      %0, %%ecx\n\t"
    "leal      %1, %%edx\n\t"
    "movaps    (%%edx), %%xmm0\n\t"
    "mulps     %2, %%xmm0\n\t"
    "movaps    %%xmm0, (%%ecx)\n\t"
    
    "addl      $16, %%ecx\n\t"
    "addl      $16, %%edx\n\t"
    "movaps    (%%edx), %%xmm0\n\t"
    "mulps     %2, %%xmm0\n\t"
    "movaps    %%xmm0, (%%ecx)\n\t"
    
    "addl      $16, %%ecx\n\t"
    "addl      $16, %%edx\n\t"
    "movaps    (%%edx), %%xmm0\n\t"
    "mulps     %2, %%xmm0\n\t"
    "movaps    %%xmm0, (%%ecx)\n\t"
    
    "addl      $16, %%ecx\n\t"
    "addl      $16, %%edx\n\t"
    "movaps    (%%edx), %%xmm0\n\t"
    "mulps     %2, %%xmm0\n\t"
    "movaps    %%xmm0, (%%ecx)\n\t"

    :"=g"(_dest)
    :"g"(_src1), "g"(v)
    :"xmm0", "ecx", "edx", "memory"
    );
#else
	_dest._11 = _src1._11 * _src2;
	_dest._12 = _src1._12 * _src2;
	_dest._13 = _src1._13 * _src2;
	_dest._14 = _src1._14 * _src2;

	_dest._21 = _src1._21 * _src2;
	_dest._22 = _src1._22 * _src2;
	_dest._23 = _src1._23 * _src2;
	_dest._24 = _src1._24 * _src2;

	_dest._31 = _src1._31 * _src2;
	_dest._32 = _src1._32 * _src2;
	_dest._33 = _src1._33 * _src2;
	_dest._34 = _src1._34 * _src2;

	_dest._41 = _src1._41 * _src2;
	_dest._42 = _src1._42 * _src2;
	_dest._43 = _src1._43 * _src2;
	_dest._44 = _src1._44 * _src2;
#endif //USE_ASM
	return 0;
}   
    
int mx_mulm( MATRIX& _dest, const MATRIX& _src1, const MATRIX& _src2 )
{   
	MATRIX ret;
	ret._11 = _src1._11 * _src2._11 + _src1._12 * _src2._21 + _src1._13 * _src2._31 + _src1._14 * _src2._41;
	ret._12 = _src1._11 * _src2._12 + _src1._12 * _src2._22 + _src1._13 * _src2._32 + _src1._14 * _src2._42;
	ret._13 = _src1._11 * _src2._13 + _src1._12 * _src2._23 + _src1._13 * _src2._33 + _src1._14 * _src2._43;
	ret._14 = _src1._11 * _src2._14 + _src1._12 * _src2._24 + _src1._13 * _src2._34 + _src1._14 * _src2._44;
    
	ret._21 = _src1._21 * _src2._11 + _src1._22 * _src2._21 + _src1._23 * _src2._31 + _src1._24 * _src2._41;
	ret._22 = _src1._21 * _src2._12 + _src1._22 * _src2._22 + _src1._23 * _src2._32 + _src1._24 * _src2._42;
	ret._23 = _src1._21 * _src2._13 + _src1._22 * _src2._23 + _src1._23 * _src2._33 + _src1._24 * _src2._43;
	ret._24 = _src1._21 * _src2._14 + _src1._22 * _src2._24 + _src1._23 * _src2._34 + _src1._24 * _src2._44;
    
	ret._31 = _src1._31 * _src2._11 + _src1._32 * _src2._21 + _src1._33 * _src2._31 + _src1._34 * _src2._41;
	ret._32 = _src1._31 * _src2._12 + _src1._32 * _src2._22 + _src1._33 * _src2._32 + _src1._34 * _src2._42;
	ret._33 = _src1._31 * _src2._13 + _src1._32 * _src2._23 + _src1._33 * _src2._33 + _src1._34 * _src2._43;
	ret._34 = _src1._31 * _src2._14 + _src1._32 * _src2._24 + _src1._33 * _src2._34 + _src1._34 * _src2._44;

	ret._41 = _src1._41 * _src2._11 + _src1._42 * _src2._21 + _src1._43 * _src2._31 + _src1._44 * _src2._41;
	ret._42 = _src1._41 * _src2._12 + _src1._42 * _src2._22 + _src1._43 * _src2._32 + _src1._44 * _src2._42;
	ret._43 = _src1._41 * _src2._13 + _src1._42 * _src2._23 + _src1._43 * _src2._33 + _src1._44 * _src2._43;
	ret._44 = _src1._41 * _src2._14 + _src1._42 * _src2._24 + _src1._43 * _src2._34 + _src1._44 * _src2._44;
	
	mx_copy( _dest, ret );
	return 0;
}

int mx_mulv( VECTOR3& _dest, const MATRIX& _src1, const VECTOR3& _src2 )
{
	VECTOR3 ret;
	ret.x = _src1._11 * _src2.x + _src1._12 * _src2.y + _src1._13 * _src2.z + _src1._14;
	ret.y = _src1._21 * _src2.x + _src1._22 * _src2.y + _src1._23 * _src2.z + _src1._24;
	ret.z = _src1._31 * _src2.x + _src1._32 * _src2.y + _src1._33 * _src2.z + _src1._34;

	_dest.x = ret.x;
	_dest.y = ret.y;
	_dest.z = ret.z;
	return 0;
}

int mx_vmul( VECTOR3& _dest, const VECTOR3& _src1, const MATRIX& _src2 )
{
#ifdef USE_ASM
	VECTOR4 v; v.x = _src1.x; v.y = _src1.y; v.z = _src1.z;
	VECTOR4 x; x.x = x.y = x.z = _src1.x;
	VECTOR4 y; y.x = y.y = y.z = _src1.y;
	VECTOR4 z; z.x = z.y = z.z = _src1.z;

	__asm__ __volatile__(
    "leal      %1, %%ecx\n\t"
    "leal      %2, %%edx\n\t"

    "movaps    48(%%edx), %%xmm0\n\t"		// _src2._4x -> xmm0

    "movaps    (%%edx), %%xmm1\n\t"			// _src2._1x -> xmm1
    "mulps     %3, %%xmm1\n\t"				// xmm1 = xmm1 * _src1.x
    "addps     %%xmm1, %%xmm0\n\t"          // xmm0 = xmm0 + xmm1

	"movaps    16(%%edx), %%xmm1\n\t"		// _src2._2x -> xmm1
    "mulps     %4, %%xmm1\n\t"				// xmm1 = xmm1 * _src1.y
    "addps     %%xmm1, %%xmm0\n\t"          // xmm0 = xmm0 + xmm1

	"movaps    32(%%edx), %%xmm1\n\t"		// _src2._3x -> xmm1
    "mulps     %5, %%xmm1\n\t"				// xmm1 = xmm1 * _src1.z
    "addps     %%xmm1, %%xmm0\n\t"          // xmm0 = xmm0 + xmm1

    "movaps    %%xmm0, %0\n\t"				// xmm0 -> v
    
    :"=g"(v)
    :"g"(v), "g"(_src2), "g"(x.v), "g"(y.v), "g"(z.v)
    :"xmm0", "xmm1", "xmm2", "ecx", "edx", "memory"
    );
    _dest.x = v.m[0];
    _dest.y = v.m[1];
    _dest.z = v.m[2];
#else
	VECTOR3 ret;
	ret.x = _src1.x * _src2._11 + _src1.y * _src2._21 + _src1.z * _src2._31 + _src2._41;
	ret.y = _src1.x * _src2._12 + _src1.y * _src2._22 + _src1.z * _src2._32 + _src2._42;
	ret.z = _src1.x * _src2._13 + _src1.y * _src2._23 + _src1.z * _src2._33 + _src2._43;

	_dest.x = ret.x;
	_dest.y = ret.y;
	_dest.z = ret.z;
#endif
	return 0;
}

int mx_det( float& _dest, const MATRIX& _src1 )
{
	_dest = _src1._11 * _src1._22 * _src1._33 * _src1._44
		  - _src1._11 * _src1._22 * _src1._34 * _src1._43
		  - _src1._11 * _src1._23 * _src1._32 * _src1._44
		  + _src1._11 * _src1._23 * _src1._34 * _src1._42
		  + _src1._11 * _src1._24 * _src1._32 * _src1._43
		  - _src1._11 * _src1._24 * _src1._33 * _src1._42
		  
		  - _src1._12 * _src1._21 * _src1._33 * _src1._44
		  + _src1._12 * _src1._21 * _src1._34 * _src1._43
		  + _src1._12 * _src1._23 * _src1._31 * _src1._44
		  - _src1._12 * _src1._23 * _src1._34 * _src1._41
		  - _src1._12 * _src1._24 * _src1._31 * _src1._43
		  + _src1._12 * _src1._24 * _src1._33 * _src1._41
		  
		  + _src1._13 * _src1._21 * _src1._32 * _src1._44
		  - _src1._13 * _src1._21 * _src1._34 * _src1._42
		  - _src1._13 * _src1._22 * _src1._31 * _src1._44
		  + _src1._13 * _src1._22 * _src1._34 * _src1._41
		  + _src1._13 * _src1._24 * _src1._31 * _src1._42
		  - _src1._13 * _src1._24 * _src1._32 * _src1._41

		  - _src1._14 * _src1._21 * _src1._32 * _src1._43
		  + _src1._14 * _src1._21 * _src1._33 * _src1._42
		  + _src1._14 * _src1._22 * _src1._31 * _src1._43
		  - _src1._14 * _src1._22 * _src1._33 * _src1._41
		  - _src1._14 * _src1._23 * _src1._31 * _src1._42
		  + _src1._14 * _src1._23 * _src1._32 * _src1._41
	;
	return 0;
}


int mx_inv( MATRIX& _dest, const MATRIX& _src1 )
{
	/*! m_inv = 1.0f / | m | X m* */

	float det;
	mx_det( det, _src1 );
	if( fabs(det) <= 0.0f )
		return -1;
	MATRIX ret;

	ret._11 =   DET3( _src1, 2, 3, 4, 2, 3, 4 );
	ret._12 = - DET3( _src1, 1, 3, 4, 2, 3, 4 );
	ret._13 =   DET3( _src1, 1, 2, 4, 2, 3, 4 );
	ret._14 = - DET3( _src1, 1, 2, 3, 2, 3, 4 );

	ret._21 = - DET3( _src1, 2, 3, 4, 1, 3, 4 );
	ret._22 =   DET3( _src1, 1, 3, 4, 1, 3, 4 );
	ret._23 = - DET3( _src1, 1, 2, 4, 1, 3, 4 );
	ret._24 =   DET3( _src1, 1, 2, 3, 1, 3, 4 );

	ret._31 =   DET3( _src1, 2, 3, 4, 1, 2, 4 );
	ret._32 = - DET3( _src1, 1, 3, 4, 1, 2, 4 );
	ret._33 =   DET3( _src1, 1, 2, 4, 1, 2, 4 );
	ret._34 = - DET3( _src1, 1, 2, 3, 1, 2, 4 );

	ret._41 = - DET3( _src1, 2, 3, 4, 1, 2, 3 );
	ret._42 =   DET3( _src1, 1, 3, 4, 1, 2, 3 );
	ret._43 = - DET3( _src1, 1, 2, 4, 1, 2, 3 );
	ret._44 =   DET3( _src1, 1, 2, 3, 1, 2, 3 );

	det = 1.0f / det;	
	mx_mulf( _dest, ret, det );

	return 0;
}

int mx_print( const MATRIX& _src1 )
{
	fprintf( stderr, " %f, %f, %f, %f\n", _src1._11, _src1._12, _src1._13, _src1._14 );
	fprintf( stderr, " %f, %f, %f, %f\n", _src1._21, _src1._22, _src1._23, _src1._24 );
	fprintf( stderr, " %f, %f, %f, %f\n", _src1._31, _src1._32, _src1._33, _src1._34 );
	fprintf( stderr, " %f, %f, %f, %f\n", _src1._41, _src1._42, _src1._43, _src1._44 );
	fprintf( stderr, "\n" );
	return 0;	
}

