#ifdef __linux

#ifndef __SERVER_SOCKET_H__
#define __SERVER_SOCKET_H__

#include "commonsocket.h"

namespace FF_Network {

class ServerSocket : public CommonSocket
{
public:
	ServerSocket(bool crypt):CommonSocket(crypt) { }
	~ServerSocket()	{ }

	int process(int events, NetMng * parent);
};

}

#endif

#endif //__linux
