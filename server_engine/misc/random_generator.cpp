#include <stdlib.h>
#include "random_generator.h"
#include "misc.h"
#include "lmisc.h"
#include <math.h>

const char RandomGenerator::className[] = "RandomGenerator";
const bool RandomGenerator::gc_delete_ = true;

Lunar<RandomGenerator>::RegType RandomGenerator::methods[] =
{
    method( RandomGenerator, c_random ),
    method( RandomGenerator, c_random_range ),
    method( RandomGenerator, c_set_seed ),
	{NULL, NULL}
};


RandomGenerator::RandomGenerator()
{
	TICK_L( cpu_tick );
	srand( Lcpu_tick );
	seed_ = (unsigned long) rand();
	//LOG(2)("seed init, seed_ = %u", seed_);
}

/*
* 算法参考：\Microsoft Visual Studio .NET 2003\Vc7\crt\src\rand.c
* 为了处理32位数, 对原算法进行了简单变化, 移位操作变成右移32位, 与操作换成32位最大值
*/
int RandomGenerator::random()
{
	//return ( ( ( ( seed_ = seed_ * 214013L + 2531011L ) >> 16 ) >> 16 ) & 0xffffffff );
	seed_ = (seed_ * 1103515245 + 12345) & 0x7fffffff;
	return seed_;
}

// return [1-_range]
int RandomGenerator::randomx( unsigned long _range )
{
	if( _range == 0 )
	{
		return 0;
	}

    float r = random() * 1.0f / 0x7fffffff;
	int tmp = floor(r * _range) + 1;
	//LOG(2)("randomx = %u, _range = %u", tmp, _range);
	return tmp;
}

int RandomGenerator::c_random( lua_State* _state )
{
	lcheck_argc( _state, 0 );

    int value = random();
    lua_pushinteger( _state, value );

    return 1;
}

int RandomGenerator::c_random_range( lua_State* _state )
{
	lcheck_argc( _state, 1 );

    unsigned long range = (unsigned long)lua_tointeger( _state, 1 );
    int value = randomx( range );
    lua_pushinteger( _state, value );

    return 1;
}

int RandomGenerator::c_set_seed( lua_State* _state )
{
	lcheck_argc( _state, 1 );

    unsigned long seed = (unsigned long)lua_tointeger( _state, 1 );
    set_seed( seed );

    return 0;
}

