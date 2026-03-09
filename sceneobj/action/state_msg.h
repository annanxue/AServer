#ifndef __STATE_MSG_H__
#define __STAET_MSG_H__

#include "log.h"

class Msg
{
public:	
	Msg( unsigned short _msg = 0, int _p1 = 0, int _p2 = 0, int _p3 = 0, int _p4 = 0, int _p5 = 0 );
	~Msg() {}
	unsigned short msg_;
	int p1_;
	int p2_;
	int p3_;
	int p4_;
    int p5_;
};

#endif


