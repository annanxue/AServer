#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#include "ar.h"
#include "autolock.h"

class Snapshot
{
public:
	Snapshot();
	~Snapshot();

	void		lock() { ar_lock_.Lock(); }
	void		unlock() { ar_lock_.Unlock(); }
	void		set_header( DPID _dpid );
	void		reset();
	void		flush();
    bool        is_packet_can_combine( unsigned short _packet_type, const char* _new_buf );
    void        log_combine( int _ctrl_id );
public:
	u_short		cb_;
	Ar			ar_;
	Mutex		ar_lock_;
public:
    unsigned short      last_send_packet_type_;                                                                 
    int                 last_send_packet_length_;                                                               
    int                 combine_packet_count_;
    int                 combine_packet_size_;
 
};

#endif //__SNAPSHOT_H__
