#ifndef __ENPOINT_H__
#define __ENPOINT_H__

#include "ffsock.h"

#define MAX_IP				16
#define MAX_DOMAIN			100

namespace FF_Network {

class EndPoint
{
public:
	sockaddr end_point_;
	void Clear();
public:
	EndPoint();
	EndPoint(const char *address, int port, int family);
	EndPoint(unsigned long address, int port, int family);
	int Address(unsigned long ip);
	int Address(const char *name);
	int Port(int port);
	int Family(int family);
	int GetPort() const;
	const char* GetString() const;
	void GetStringIP( char* buf ) const;
#ifdef __linux
	in_addr_t GetIP() const { return ((struct sockaddr_in*)&end_point_)->sin_addr.s_addr; }
#endif
	operator struct sockaddr_in*() { return (struct sockaddr_in*)&end_point_; }
	operator sockaddr* () { return &end_point_; }
	operator const sockaddr* () const { return &end_point_; }
	};

}

#endif

