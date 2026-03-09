#ifndef __UTIL_3D_H__
#define __UTIL_3D_H__

#include "3dmath.h"

int	is_touch_ray_triangle( const VECTOR3& _v0, const VECTOR3& _v1, const VECTOR3& _v2,
							const VECTOR3& _v_orig, const VECTOR3& _v_dir, 
							float& _dist );

int is_touchAABB( const VECTOR3 &vmin1, const VECTOR3 &vmax1,  
					 const VECTOR3 &vmin2, const VECTOR3 &vmax2 );
					 
int	is_touch_AABB_line( const VECTOR3& _v_min, const VECTOR3& _v_max, 
							const VECTOR3& _v1, const VECTOR3& _v2, 
							VECTOR3& _v_intersect );
						 
int	is_touch_OBB_line( const VECTOR3& _v_min, const VECTOR3& _v_max, const MATRIX& _mat,
							const VECTOR3& _v1, const VECTOR3& _v2, VECTOR3& _v_intersect );

int	is_touch_OBB_line_with_inv( const VECTOR3& _v_min, const VECTOR3& _v_max,
						const MATRIX& _mat, const MATRIX& _mat_inv, 
						const VECTOR3& v1, const VECTOR3& v2, VECTOR3& _v_intersect );

#endif
