#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include "ffpoll.h"

#ifdef __linux

int poll_create(poll_data & pdata, int max_con)
{
	int _epfd = epoll_create(max_con);
	if ( _epfd > 0 )
	{
		pdata.epfd_ = _epfd;
		pdata.max_con_ = max_con;
		pdata.result_event_ = new epoll_event[max_con];
		signal(SIGPIPE, SIG_IGN);
	}
	return _epfd;
}

int poll_add(poll_data & pdata, int fd, int mask)
{
	struct epoll_event ee;
    memset( &ee, 0, sizeof(ee) );
	ee.events = mask;
	ee.data.fd = fd;
	int ret = epoll_ctl(pdata.epfd_, EPOLL_CTL_ADD, fd, &ee);
	return ret;
}

int poll_mod(poll_data & pdata, int fd, int mask)
{
	struct epoll_event ee;
    memset( &ee, 0, sizeof(ee) );
	ee.events = mask;
	ee.data.fd = fd;
	int ret = epoll_ctl(pdata.epfd_, EPOLL_CTL_MOD, fd, &ee);
	return ret;
}

int poll_del(poll_data & pdata, int fd)
{
	struct epoll_event ee;
    memset( &ee, 0, sizeof(ee) );
	ee.events = 0;
	ee.data.fd = fd;
	int ret = epoll_ctl(pdata.epfd_, EPOLL_CTL_DEL, fd, &ee);
	return ret;
}

int poll_wait(poll_data & pdata, int timeval)
{
	assert(pdata.result_event_);
	assert(pdata.max_con_ > 0);
	pdata.result_num_ = epoll_wait(pdata.epfd_, pdata.result_event_, pdata.max_con_, timeval);
	pdata.event_idx_ = 0;
	return pdata.result_num_;
}

int poll_destroy(poll_data & pdata)
{
	close(pdata.epfd_);
	if (pdata.result_event_)
	{
		delete [] pdata.result_event_;
		pdata.result_event_ = NULL;
	}
	return 0;
}

int poll_event(poll_data & pdata, int * fd, int * events)
{
	if ( pdata.event_idx_ < pdata.result_num_ )
	{
		struct epoll_event & ee = pdata.result_event_[pdata.event_idx_++];
		*events = ee.events;
		*fd = ee.data.fd;
		return 0;
	}
	return -1;
}

#elif defined(_WIN32)

int poll_create(poll_data & pdata, int max_con)
{
	pdata.fd_ = -1;
	pdata.result_ = 0;
	return 0;
}

int poll_add(poll_data & pdata, int fd, int mask)
{
	pdata.fd_ = fd;
	pdata.mask_ = mask;
	return 0;
}

int poll_mod(poll_data & pdata, int fd, int mask)
{
	pdata.mask_ = mask;
	return 0;
}

int poll_del(poll_data & pdata, int fd)
{
	pdata.fd_ = DSOCKERR;
	return 0;
}

int poll_wait(poll_data & pdata, int timeval)
{
	if ( DSOCKERR == pdata.fd_ )
	{
		ff_sleep(timeval);
		return -1;
	}

	struct timeval tv;
	
	FD_ZERO(&pdata.fdr_);
	FD_ZERO(&pdata.fdw_);
	FD_ZERO(&pdata.fde_);
	if ( pdata.mask_ & poll_data::EVENT_IN )
		FD_SET(pdata.fd_, &pdata.fdr_);
	if ( pdata.mask_ & poll_data::EVENT_OUT )
		FD_SET(pdata.fd_, &pdata.fdw_);
	if ( pdata.mask_ & poll_data::EVENT_ERR )
		FD_SET(pdata.fd_, &pdata.fde_);
	tv.tv_sec = timeval / 1000;
	tv.tv_usec = timeval - tv.tv_sec;
	
	int ret = select(pdata.fd_ + 1, &pdata.fdr_, &pdata.fdw_, &pdata.fde_, &tv);

	if ( FD_ISSET(pdata.fd_, &pdata.fdr_) )
		pdata.result_ |= poll_data::EVENT_IN;
	if ( FD_ISSET(pdata.fd_, &pdata.fdw_) )
		pdata.result_ |= poll_data::EVENT_OUT;
	if ( FD_ISSET(pdata.fd_, &pdata.fde_) )
		pdata.result_ |= poll_data::EVENT_ERR;
	return ret;
}
int poll_destroy(poll_data & pdata)
{
	return 0;
}

int poll_event(poll_data & pdata, int * fd, int * events)
{
	if ( pdata.result_ )
	{
		*fd = pdata.fd_;
		*events = pdata.result_;
		pdata.result_ = 0;
		return 0;
	}
	return -1;
}

#endif
