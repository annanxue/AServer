//
// $Id: ffsock.c 22901 2007-05-09 04:42:21Z loon $
//

#include "ffsock.h"

int ff_netstart(int v)
{
#ifdef _WIN32
	struct WSAData wsa;
	WSAStartup((unsigned short)v, &wsa);	
	
	/*!< 这里建议最好还是判断一下WSAStartup的返回值，不过要注意两点：
	1.不要使用WSAGetLastError,因为在失败的情况下，WS2_32.DLL根本就没有加载进来，这时
	client data area还未曾建立，而last error information是存放于此的。
	2.如果想利用wsa的vVersion属性来判断DLL的版本那就要注意：如果DLL仅支持v(比如：2.2)
	以上的版本号（不包括v），这时虽然DLL加载失败，但vVersion返回的值却为v，所以想用它
	来做判断就会失真。
	*/
#endif
	return 0;
}

int ff_netclose(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}

int ff_socket(int family, int type, int protocol)
{
	return (int)socket(family, type, protocol);
}

int ff_close(int sock)
{
	if (sock < 0) return 0;
#ifdef __linux
	close(sock);
#else
	closesocket(sock);
#endif
	return 0;
}

int ff_connect(int sock, const struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return connect(sock, addr, len);
}

int ff_shutdown(int sock, int mode)
{
	return shutdown(sock, mode);
}

int ff_bind(int sock, const struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return bind(sock, addr, len);
}

int ff_listen(int sock, int count)
{
	return listen(sock, count);
}

int ff_accept(int sock, struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return accept(sock, addr, &len);
}

int ff_errno(void)
{
#ifdef __linux
	return errno;
#else
	return WSAGetLastError();
#endif
}

int ff_send(int sock, const void *buf, long size, int mode)
{
	return send(sock, (char*)buf, size, mode);
}

int ff_recv(int sock, void *buf, long size, int mode)
{
	return recv(sock, (char*)buf, size, mode);
}

int ff_sendto(int sock, const void *buf, long size, int mode, const struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return sendto(sock, (char*)buf, size, mode, addr, len);
}

int ff_recvfrom(int sock, void *buf, long size, int mode, struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return recvfrom(sock, (char*)buf, size, mode, addr, &len);
}

int ff_ioctl(int sock, long cmd, unsigned long *argp)
{
#ifdef __linux
	return ioctl(sock, cmd, argp);
#else
	return ioctlsocket(sock, cmd, argp);
#endif
}

int ff_setsockopt(int sock, int level, int optname, const char *optval, int optlen)
{
	socklen_t len = optlen;
	return setsockopt(sock, level, optname, optval, len);
}

int ff_getsockopt(int sock, int level, int optname, char *optval, int *optlen)
{
	socklen_t len = (optlen)? *optlen : 0;
	int retval;
	retval = getsockopt(sock, level, optname, optval, &len);
	if (optlen) *optlen = len;

	return retval;
}

int ff_sockname(int sock, struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return getsockname(sock, addr, &len);
}

int ff_peername(int sock, struct sockaddr *addr)
{
	socklen_t len = sizeof(struct sockaddr);
	return getpeername(sock, addr, &len);
}

