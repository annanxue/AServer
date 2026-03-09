/*
 * FlowRate is used to control data flow rate.
 * 
 */

#ifdef __linux

#ifndef __FLOW_RATE_H__
#define __FLOW_RATE_H__

#define MAX_FLOW_FRAMES 100
#define MAX_FLOW_BYTES 81920

#include <stdio.h>
#include "ffsock.h"

namespace FF_Network {

class FlowRate
{
public:
	FlowRate();
	~FlowRate();

public:
	void set_over_flow( bool over_flow );
	void add(int bytes);
	bool get_over_flow();
	bool is_over_flow_with_frame( int bytes );
	bool is_over_flow( int bytes );
	
private:
	u_long start_frm_;
	int current_bytes_;
	bool over_flow_;
};

}

#endif

#endif //__linux
