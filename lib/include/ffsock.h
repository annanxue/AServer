#ifndef __FF_SOCK_H__
#define __FF_SOCK_H__

#include "ffsys.h"

#if defined(__linux) || defined(__linux__) || defined(__APPLE__) || \
    defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
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

#define DSOCKERR -1
#define DWOULDBLOCK EWOULDBLOCK
#define DCONNECTERR EINPROGRESS

#elif defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
/* select() 默认仅支持 64 个 fd，服务端需要更高上限 */
#ifndef FD_SETSIZE
#define FD_SETSIZE 4096
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define DSOCKERR SOCKET_ERROR
#define DWOULDBLOCK WSAEWOULDBLOCK
#define DCONNECTERR WSAEINPROGRESS
typedef int socklen_t;
#ifndef in_addr_t
typedef unsigned long in_addr_t;
#endif
#ifndef ulong
typedef unsigned long ulong;
#endif

#else
#error Unsupported platform: only Windows, Linux and BSD/macOS are supported
#endif

#define CONNECT_TIMEOUT 10

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

#endif /* __FF_SOCK_H__ */
