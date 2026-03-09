#ifndef __FFPOLL_H__
#define __FFPOLL_H__

#ifdef __linux
#include <sys/epoll.h>
#endif

#include "ffsys.h"
#include "ffsock.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux

typedef struct {
	enum { EVENT_IN = EPOLLIN, EVENT_OUT = EPOLLOUT, EVENT_ERR = EPOLLERR };
	int epfd_;
	int max_con_;
	int result_num_;
	int event_idx_;
	struct epoll_event * result_event_;
} poll_data;

#elif defined(_WIN32)

typedef struct {
	enum { EVENT_IN = 1, EVENT_OUT = 2, EVENT_ERR = 4 };
	int fd_;
	int mask_;
	fd_set fdr_;
	fd_set fdw_;
	fd_set fde_;
	int result_;
} poll_data;

#else
#error Unknow platform, only support linux and win32
#endif

int poll_create(poll_data & pdata, int max_con);

int poll_add(poll_data & pdata, int fd, int mask);

int poll_mod(poll_data & pdata, int fd, int mask);

int poll_del(poll_data & pdata, int fd);

int poll_wait(poll_data & pdata, int timeval);

int poll_destroy(poll_data & pdata);

int poll_event(poll_data & pdata, int * fd, int * events);

#ifdef __cplusplus
}
#endif

#endif
