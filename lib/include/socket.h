#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "ffsock.h"

namespace FF_Network {

class Socket
{
protected:
#ifdef _WIN32
	static int initcnt;
#endif
	int sock;
	void init();

public:
	Socket(int _sock);
	Socket(int family, int type, int protocol);
	virtual ~Socket();
	void start_up();
	int Send(const void *buf, long size, int mode);
	int Recv(void *buf, long size, int mode);
	int SendTo(const void *buf, long size, int mode, const sockaddr *addr);
	int RecvFrom(void *buf, long size, int mode, sockaddr *addr);
public:
	int Ioctl(long cmd, unsigned long *argp);
	int SetOption(int level, int optname, const char *optval, int optlen);
	int GetOption(int level, int optname, char *optval, int *optlen);
	void SetHandle(int _sock);
	int Handle() const;
	int Sockname(int sock, struct sockaddr *addr);
	int Peername(int sock, struct sockaddr *addr);
public:	
	int Open(int family, int type, int protocol);
	int Close();
	int Connect(const sockaddr *addr);
	int Shutdown(int mode);
	int Bind(const sockaddr *addr);
	int Listen(int count);
	int Accept(sockaddr *addr);
	int ErrNo();

};

} //namespace FF_Network

#endif

