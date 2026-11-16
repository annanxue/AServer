#ifndef __FFPOLL_H__
#define __FFPOLL_H__

#include "ffsys.h"
#include "ffsock.h"

#ifdef ASERVER_LINUX
#include <sys/epoll.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FFPOLL_MAX_FDS 4096

#ifdef ASERVER_LINUX

typedef struct {
    enum { EVENT_IN = EPOLLIN, EVENT_OUT = EPOLLOUT, EVENT_ERR = EPOLLERR };
    int epfd_;
    int max_con_;
    int result_num_;
    int event_idx_;
    struct epoll_event *result_event_;
} poll_data;

#elif defined(ASERVER_BSD)

typedef struct {
    enum { EVENT_IN = POLLIN, EVENT_OUT = POLLOUT, EVENT_ERR = POLLERR };
    int max_con_;
    int count_;
    int result_num_;
    int event_idx_;
    int fd_[FFPOLL_MAX_FDS];
    int mask_[FFPOLL_MAX_FDS];
    int result_fd_[FFPOLL_MAX_FDS];
    int result_event_[FFPOLL_MAX_FDS];
    struct pollfd *pollfds_;
} poll_data;

#elif defined(ASERVER_WINDOWS)

typedef struct {
    enum { EVENT_IN = 1, EVENT_OUT = 2, EVENT_ERR = 4 };
    int max_con_;
    int count_;
    int result_num_;
    int event_idx_;
    int fd_[FFPOLL_MAX_FDS];
    int mask_[FFPOLL_MAX_FDS];
    int result_fd_[FFPOLL_MAX_FDS];
    int result_event_[FFPOLL_MAX_FDS];
    fd_set fdr_;
    fd_set fdw_;
    fd_set fde_;
} poll_data;

#else
#error Unsupported platform: only Windows, Linux and BSD/macOS are supported
#endif

int poll_create(poll_data & pdata, int max_con);
int poll_add(poll_data & pdata, int fd, int mask);
int poll_mod(poll_data & pdata, int fd, int mask);
int poll_del(poll_data & pdata, int fd);
int poll_wait(poll_data & pdata, int timeval);
int poll_destroy(poll_data & pdata);
int poll_event(poll_data & pdata, int *fd, int *events);

#ifdef __cplusplus
}
#endif

#endif /* __FFPOLL_H__ */
