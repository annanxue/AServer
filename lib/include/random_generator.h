#ifndef __RANDOM_GENERATOR_H__
#define __RANDOM_GENERATOR_H__

#include "lunar.h"

/*
* 我们经常需要用到几个互不干涉的平均分布的随机数发生器，使用C库的rand()函数并不
* 能保证这一点，原因是所有rand()函数共用一个种子。如下的代码并不能获得期待的概率
* if( rand() % 100 < 50 )	// 50%
  {
  	...
  }
  
  if( rand() % 100 < 20 )	// 20%
  {
  	...
  }
  这两个rand()会互相影响，每个的分布都不是均匀的。

* 用类使种子在对象的范围内，每个生成随机数的地方使用一个单独的对象，这样才能比较
* 精确的保证平均分布。代码示例如下：
* static RandomGenerator r1, r2;
* if( r1.random( 100 ) < 50 )	// 50%
  {
    ...
  }
  if( r2.random( 100 ) < 20 )	// 20%
  {
  	...
  }
*/


class RandomGenerator
{
public:
	RandomGenerator();
	virtual ~RandomGenerator() 						{ }
	void set_seed( unsigned long _seed )			{ seed_ = _seed; }
	unsigned long get_seed() const					{ return seed_; }
	int random();
	int randomx( unsigned long _range );

protected:

private:
	unsigned long seed_;

public:
    static const char   className[];
    static              Lunar<RandomGenerator>::RegType methods[];
    static const bool   gc_delete_;
public:	
    int c_random( lua_State* _state );
    int c_random_range( lua_State* _state );
    int c_set_seed( lua_State* _state );
};


#endif
