#ifdef __linux

#include "flow_rate.h"

namespace FF_Network {

extern int g_network_frm;

FlowRate::FlowRate()
{
	start_frm_ = g_network_frm;
	current_bytes_ = 0;
	over_flow_ = false;
}

FlowRate::~FlowRate()
{
}

bool FlowRate::get_over_flow()
{
	return over_flow_;
}

void FlowRate::set_over_flow( bool over_flow )
{
	over_flow_ = over_flow;
}

void FlowRate::add( int bytes )
{
	current_bytes_ += bytes;
}

bool FlowRate::is_over_flow_with_frame( int bytes )
{
	u_long dwFrm = g_network_frm;
	current_bytes_ += bytes;
	if( dwFrm - start_frm_ > MAX_FLOW_FRAMES )
	{
		start_frm_ = dwFrm;
		if( current_bytes_ > MAX_FLOW_BYTES )
		{
			over_flow_ = true;
		}

		current_bytes_ = 0;			
	}
	
	return over_flow_;
}

bool FlowRate::is_over_flow( int bytes )
{
	current_bytes_ += bytes;
	if( current_bytes_ > MAX_FLOW_BYTES )
		over_flow_ = true;

	return over_flow_;
}

}

#endif //__linux
