#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "socket.h"

namespace FF_Network {

#ifdef _WIN32
int Socket::initcnt = 0;
#endif

void Socket::init()
{
	sock = DSOCKERR;
#ifdef _WIN32
	if (initcnt++ == 0) ff_netstart(0x0202);
	/*!< 
	在winsock的DLL内部维持着一个计数器，只有第一次调用WSAStartup才真正装载DLL，以后的调用
	只是简单的增加计数器，而WSACleanup函数的功能则刚好相反，每调用一次使计数器减1，当计数
	器减到0时，DLL就从内存中被卸载！因此，调用了多少次WSAStartup，就应相应的调用多少次的
	WSACleanup。
	
	不过，此处仍然进行计数控制的目的是考虑到在ff_netstart中，除了进行WSAStartup之外，还有
	可能做其它的操作，况且即便是WSAStartup，也不让它去尝试装载DLL，因为这也是需要开销的。
	*/
#endif
}

void Socket::start_up()
{
	unsigned long noblock = 1;
	unsigned long revalue = 1;
	// set socket to nonblock
	Ioctl(FIONBIO, &noblock);
	SetOption(SOL_SOCKET, SO_REUSEADDR, (char*)&revalue, sizeof(revalue));
}

Socket::Socket(int _sock)
{
	init();
	sock = _sock;
	start_up();
}

Socket::Socket(int family, int type, int protocol)
{
	init();
	Open(family, type, protocol);
	start_up();
}

Socket::~Socket()
{
	Close();
#ifdef _WIN32
	if (--initcnt == 0) ff_netclose();
#endif
}

int Socket::Open(int family, int type, int protocol)
{
	Close();
	sock = ff_socket(family, type, protocol);
	if (sock < 0) return -1;
	return 0;
}

int Socket::Close()
{
	if (sock < 0) return 0;
	ff_close(sock);
	sock = -1;
	return 0;
}

void Socket::SetHandle(int _sock)
{
	sock = _sock;
}

int Socket::Handle() const
{
	return sock;
}

int Socket::Send(const void *buf, long size, int mode)
{
	return ff_send(sock, (char*)buf, size, mode);
}

int Socket::Recv(void *buf, long size, int mode)
{
	return ff_recv(sock, (char*)buf, size, mode);
}

int Socket::RecvFrom(void *buf, long size, int mode, sockaddr *addr)
{
	return ff_recvfrom(sock, (char*)buf, size, mode, addr);
}

int Socket::SendTo(const void *buf, long size, int mode, const sockaddr *addr)
{
	return ff_sendto(sock, (char*)buf, size, mode, addr);
}

int Socket::Connect(const sockaddr *addr)
{
	if ( ( DSOCKERR == ff_connect(sock, addr) ) && ( DCONNECTERR == ff_errno() ) )
	{
        /*
		fd_set fdw,fde;
		struct timeval tv;

		FD_ZERO(&fdw);
		FD_ZERO(&fde);
		FD_SET(sock, &fdw);
		FD_SET(sock, &fde);
		tv.tv_sec = CONNECT_TIMEOUT;
		tv.tv_usec = 0;

		int ret = select(sock + 1, NULL, &fdw, &fde, &tv);

		if( ret > 0 && FD_ISSET(sock, &fdw) && !FD_ISSET(sock, &fde) )
		{
#ifdef __linux
			int val,len = sizeof(val);
			if( !GetOption(SOL_SOCKET, SO_ERROR, (char*)&val, &len) && val )
			{
				errno = val;
				return -1;
			}
#endif
			return 0;
		}
        */
        pollfd fds[1];
        fds[0].fd = sock;
        fds[0].events = POLLOUT;

        int ret = poll( fds, 1, CONNECT_TIMEOUT*1000 );
        if( ret > 0 )
        {
#ifdef __linux
			int val,len = sizeof(val);
			if( !GetOption(SOL_SOCKET, SO_ERROR, (char*)&val, &len) && val )
			{
				errno = val;
				return -1;
			}
#endif
            return 0;
        }
	}
	return -1;
}

int Socket::Shutdown(int mode)
{
	return ff_shutdown(sock, mode);
}

int Socket::Bind(const sockaddr *addr)
{
	return ff_bind(sock, addr);
}

int Socket::Listen(int count)
{
	return ff_listen(sock, count);
}

int Socket::Accept(sockaddr *addr)
{
	return ff_accept(sock, addr);
}

int Socket::Ioctl(long cmd, unsigned long *argp)
{
	return ff_ioctl(sock, cmd, argp);
}

int Socket::SetOption(int level, int optname, const char *optval, int optlen)
{
	return ff_setsockopt(sock, level, optname, optval, optlen);
}

int Socket::GetOption(int level, int optname, char *optval, int *optlen)
{
	return ff_getsockopt(sock, level, optname, optval, optlen);
}

int Socket::Sockname(int sock, struct sockaddr *addr)
{
	return ff_sockname(sock, addr);
}

int Socket::Peername(int sock, struct sockaddr *addr)
{
	return ff_peername(sock, addr);
}

int Socket::ErrNo()
{
	return ff_errno();
}

} //namespace FF_Network
