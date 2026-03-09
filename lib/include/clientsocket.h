#ifndef __CLIENT_SOCKET_H__
#define __CLIENT_SOCKET_H__

#include "commonsocket.h"

namespace FF_Network {

class NetMng;

class ClientSocket : public CommonSocket
{
public:
	ClientSocket(bool crypt):CommonSocket(crypt) { }
	ClientSocket(int sock, bool crypt):CommonSocket(sock, crypt) { }
	~ClientSocket() { }

	int		process(int events, NetMng * parent);
};

}

#endif // __CLIENT_SOCKET_H__
