#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "endpoint.h"
#include "ffsys.h"

namespace FF_Network {

EndPoint::EndPoint()
{
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
	Clear();
	addr->sin_family = AF_INET;
}

void EndPoint::Clear()
{
	memset(&end_point_, 0, sizeof(end_point_));
}

int EndPoint::Port(int port)
{
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
	addr->sin_port = htons(port);
	return 0;
}

int EndPoint::Family(int family)
{
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
	addr->sin_family = family;
	return 0;
}

int EndPoint::Address(unsigned long ip)
{
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
#ifdef __linux
	addr->sin_addr.s_addr = htonl(ip);
#else
	addr->sin_addr.S_un.S_addr = htonl(ip);
#endif
	return 0;
}

int EndPoint::Address(const char *name)
{
	int is_name = 0;

	for (int i=0; name[i]; ++i)
	{
		if (!((name[i] >= '0' && name[i] <= '9') || name[i] == '.'))
		{
			is_name = 1;
			break;
		}
	}
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;

	if (is_name)
	{
		struct hostent * he = ::gethostbyname(name);
		if (he == NULL)
		{
			Clear();
			return -1;
		}
		if (he->h_length != 4) return -2;
		memcpy((char*)&(addr->sin_addr), he->h_addr, he->h_length);
	}
	else
	{
#ifdef __linux
		addr->sin_addr.s_addr = inet_addr(name);
#else
		addr->sin_addr.S_un.S_addr = inet_addr(name);
#endif
	}	
	return 0;
}

EndPoint::EndPoint(const char *address, int port, int family)
{
	Clear();
	Address(address);
	Port(port);
	Family(family);
}

EndPoint::EndPoint(unsigned long address, int port, int family)
{
	Clear();
	Address(address);
	Port(port);
	Family(family);
}

int EndPoint::GetPort() const
{
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
	return htons(addr->sin_port);
}

const char* EndPoint::GetString() const
{
	static char str_ip[32];
	static int ipb[5];
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
	unsigned char *bytes = NULL;
#ifdef __linux
	bytes = (unsigned char*)&(addr->sin_addr.s_addr);
#else
	bytes = (unsigned char*)&(addr->sin_addr.S_un);	
#endif
	for (int i = 0; i < 4; ++i) ipb[i] = (int) bytes[i];
	ipb[4] = htons(addr->sin_port);
	ff_snprintf( str_ip, 32,"%d.%d.%d.%d:%d", ipb[0], ipb[1], ipb[2], ipb[3], ipb[4]);
	return str_ip;
}

void EndPoint::GetStringIP( char* buf ) const
{
	assert(buf);

	int ipb[4];
	struct sockaddr_in *addr = (struct sockaddr_in*)&end_point_;
	unsigned char *bytes = NULL;
#ifdef __linux
	bytes = (unsigned char*)&(addr->sin_addr.s_addr);
#else
	bytes = (unsigned char*)&(addr->sin_addr.S_un);	
#endif
	for (int i = 0; i < 4; ++i) ipb[i] = (int) bytes[i];
	ff_snprintf( buf, MAX_IP,"%d.%d.%d.%d", ipb[0], ipb[1], ipb[2], ipb[3] );
}


}
