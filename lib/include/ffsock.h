#ifndef __FF_SOCK_H__
#define __FF_SOCK_H__

#if defined(__linux)
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define DSOCKERR		-1
#define DWOULDBLOCK		EWOULDBLOCK
#define DCONNECTERR		EINPROGRESS

#elif defined(_WIN32)
#include <windows.h>
#include <winsock.h>

#define DSOCKERR		SOCKET_ERROR
#define DWOULDBLOCK		WSAEWOULDBLOCK
#define DCONNECTERR		WSAEWOULDBLOCK

#define socklen_t long
#else
#error Unknow platform, only support linux and win32
#endif

#define CONNECT_TIMEOUT	10

#ifdef __cplusplus
extern "C" {
#endif

int ff_netstart(int v);
int ff_netclose(void);
int ff_socket(int family, int type, int protocol);
int ff_close(int sock);
int ff_connect(int sock, const struct sockaddr *addr);
int ff_shutdown(int sock, int mode);
int ff_bind(int sock, const struct sockaddr *addr);
int ff_listen(int sock, int count);
int ff_accept(int sock, struct sockaddr *addr);
int ff_errno(void);
int ff_send(int sock, const void *buf, long size, int mode);
int ff_recv(int sock, void *buf, long size, int mode);
int ff_sendto(int sock, const void *buf, long size, int mode, const struct sockaddr *addr);
int ff_recvfrom(int sock, void *buf, long size, int mode, struct sockaddr *addr);
int ff_ioctl(int sock, long cmd, unsigned long *argp);
int ff_setsockopt(int sock, int level, int optname, const char *optval, int optlen);
int ff_getsockopt(int sock, int level, int optname, char *optval, int *optlen);
int ff_sockname(int sock, struct sockaddr *addr);
int ff_peername(int sock, struct sockaddr *addr);

#ifdef __cplusplus
}
#endif

#endif

