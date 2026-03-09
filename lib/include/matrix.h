#ifndef __MATRIX_H__
#define __MATRIX_H__

#ifdef USE_ASM
#include <xmmintrin.h>
#endif
#include "vector.h"

/*!
* 16字节对齐，便于以后使用sse指令优化
*/
typedef struct _MATRIX
{
    union {
#ifdef USE_ASM
    	__declspec(align(16)) __m128 v[4];		//SIMD的存储方式
#endif
        struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
    };
} MATRIX;//  __attribute__ ((aligned (16)));

#ifdef __cplusplus
extern "C" {
#endif 

/*! 基本的4X4矩阵运算
* 出于性能的考虑，参数指针的有效性由调用者保证；
*/
int mx_init( MATRIX& _dest, float* _src1 );
int mx_initn( MATRIX& _dest, float, float, float, float,
							float, float, float, float, 
							float, float, float, float, 
							float, float, float, float );
int mx_identity( MATRIX& _dest );
int mx_copy( MATRIX& _dest, const MATRIX& _src1 );
int mx_add( MATRIX& _dest, const MATRIX& _src1, const MATRIX& _src2 );
int mx_sub( MATRIX& _dest, const MATRIX& _src1, const MATRIX& _src2 );
int mx_mulf( MATRIX& _dest, const MATRIX& _src1, const float _src2 );
int mx_mulm( MATRIX& _dest, const MATRIX& _src1, const MATRIX& _src2 );
int mx_mulv( VECTOR3& _dest, const MATRIX& _src1, const VECTOR3& _src2 );
int mx_vmul( VECTOR3& _dest, const VECTOR3& _src1, const MATRIX& _src2 );
int mx_det( float& _dest, const MATRIX& _src1 );
int mx_inv( MATRIX& _dest, const MATRIX& _src1 );

int mx_print( const MATRIX& _src1 );

#ifdef __cplusplus
}
#endif 

#endif
