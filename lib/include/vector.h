#ifndef __VECTOR_H__
#define __VECTOR_H__

#ifdef USE_ASM
#include <xmmintrin.h>
#endif

/*!
* 16字节对齐，便于以后使用sse指令优化
*/
typedef struct _DVECTOR3
{
	union {
		struct {
			float x;
			float y;
			float z;
		};
		float m[3];
	};
	_DVECTOR3( float _x, float _y, float _z):x(_x),y(_y),z(_z) {}
	_DVECTOR3() {}
} VECTOR, VECTOR3;//  __attribute__ ((aligned (16)));


/*!
* 16字节对齐，便于以后使用sse指令优化
*/
typedef struct _DVECTOR4
{
	union {
#ifdef USE_ASM
    	__declspec(align(16)) __m128 v;			//SIMD的存储方式
#endif
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		float m[4];
	};
} VECTOR4;// __attribute__ ((aligned (16)));

#ifdef __cplusplus
extern "C" {
#endif 

/*! 基本的3元，4元矢量运算
* 出于性能的考虑，参数指针的有效性由调用者保证；
*/
int vc3_init( VECTOR3& _dest, float _x, float _y, float _z );
int vc3_copy( VECTOR3& _dest, const VECTOR3& _src1);
int vc3_add( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2 );
int vc3_sub( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2 );
int vc3_mul( VECTOR3& _dest, const VECTOR3& _src1, float _src2 );
int vc3_norm( VECTOR3& _dest, const VECTOR3& _src1 , float _len = 1.0f);
int vc3_cross( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2 );
float vc3_dot( const VECTOR3& _src1, const VECTOR3& _src2 );
float vc3_len( const VECTOR3& _src1 );
float vc3_xzlen( const VECTOR3& _src1 );
float vc3_lensq( const VECTOR3& _src1 );
float vc3_xzlensq( const VECTOR3& _src1 );
float vc3_xzlenq( const VECTOR3& _src1 );
int vc3_equal( const VECTOR3& _src1, const VECTOR3& _src2 );
int vc3_print( const VECTOR3& _src1 );
int vc3_intrp( VECTOR3& _dest, const VECTOR3& _src1, const VECTOR3& _src2, float _rate );
int vc3_iszero(const VECTOR3& _src1);

int vc4_init( VECTOR4&, float _x, float _y, float _z, float _w );
int vc4_copy( VECTOR4& _dest, const VECTOR4& _src1);
int vc4_add( VECTOR4& _dest, const VECTOR4& _src1, const VECTOR4& _src2 );
int vc4_sub( VECTOR4& _dest, const VECTOR4& _src1, const VECTOR4& _src2 );
int vc4_mul( VECTOR4& _dest, const VECTOR4& _src1, float _src2 );
int vc4_norm( VECTOR4& _dest, const VECTOR4& _src1 );
int vc4_cross( VECTOR4& _dest, const VECTOR4& _src1, const VECTOR4& _src2 );
float vc4_dot( const VECTOR4& _src1, const VECTOR4& _src2 );
float vc4_len( const VECTOR4& _src1 );
float vc4_lensq( const VECTOR4& _src1 );
int vc4_print( const VECTOR4& _src1 );

#ifdef __cplusplus
}
#endif 

#ifdef __cplusplus
VECTOR3 operator+( const VECTOR3& _src1, const VECTOR3& _src2 );
VECTOR3 operator-( const VECTOR3& _src1, const VECTOR3& _src2 );
VECTOR3 operator*( const VECTOR3& _src1, float _src2 );
int operator==( const VECTOR3& _src1, const VECTOR3& _src2 );
int operator!=( const VECTOR3& _src1, const VECTOR3& _src2 );
#endif


#endif
