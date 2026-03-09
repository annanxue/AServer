#ifndef __3DMATH_H__
#define __3DMATH_H__

#include <math.h>
#include "vector.h"
#include "matrix.h"

#define DX_PI 3.1415926f

float vc3_get_angle( const VECTOR3& src1 );
float vc3_get_angle_x( const VECTOR3& src1 );

int mx_3d_scaling( MATRIX& dest, float sx, float sy, float sz );
int mx_3d_rotation_x( MATRIX& dest, float rx );
int mx_3d_rotation_y( MATRIX& dest, float ry );
int mx_3d_rotation_z( MATRIX& dest, float rz );
int mx_3d_translation( MATRIX& dest, float tx, float ty, float tz );

//! misc函数
#define d3d_to_radian( degree ) ((degree) * (DX_PI / 180.0f))
#define d3d_to_degree( radian ) ((radian) * (180.0f / DX_PI))

inline void d3d_transform_coord( VECTOR3& dest, const VECTOR3& src1, const MATRIX& src2 ) { mx_vmul( dest, src1, src2 ); }

float to_radian( float );

float SIN( float d );
float COS( float d );
VECTOR3& GET_DIR( float d );
void init_t_math();

float get_degree( VECTOR3 _dest_pos, VECTOR3 _src_pos );
float get_degree_x( const VECTOR3& _dest_pos, const VECTOR3& _src_pos );

VECTOR3 velocity_to_vec( float _angle, float _velocity );

bool vc3_check_out_range( VECTOR3& _src, VECTOR3 _dest, float _range );

#endif
