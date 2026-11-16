#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include "ffpoll.h"

#ifdef ASERVER_LINUX

int poll_create(poll_data & pdata, int max_con)
{
    int epfd = epoll_create(max_con);
    if (epfd > 0)
    {
        pdata.epfd_ = epfd;
        pdata.max_con_ = max_con;
        pdata.result_event_ = new epoll_event[max_con];
        signal(SIGPIPE, SIG_IGN);
    }
    return epfd;
}

int poll_add(poll_data & pdata, int fd, int mask)
{
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.events = mask;
    ee.data.fd = fd;
    return epoll_ctl(pdata.epfd_, EPOLL_CTL_ADD, fd, &ee);
}

int poll_mod(poll_data & pdata, int fd, int mask)
{
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.events = mask;
    ee.data.fd = fd;
    return epoll_ctl(pdata.epfd_, EPOLL_CTL_MOD, fd, &ee);
}

int poll_del(poll_data & pdata, int fd)
{
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.events = 0;
    ee.data.fd = fd;
    return epoll_ctl(pdata.epfd_, EPOLL_CTL_DEL, fd, &ee);
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
    delete [] pdata.result_event_;
    pdata.result_event_ = NULL;
    return 0;
}

int poll_event(poll_data & pdata, int *fd, int *events)
{
    if (pdata.event_idx_ < pdata.result_num_)
    {
        struct epoll_event & ee = pdata.result_event_[pdata.event_idx_++];
        *events = ee.events;
        *fd = ee.data.fd;
        return 0;
    }
    return -1;
}

#elif defined(ASERVER_BSD)

static int find_fd(poll_data & pdata, int fd)
{
    for (int i = 0; i < pdata.count_; ++i)
    {
        if (pdata.fd_[i] == fd) return i;
    }
    return -1;
}

static short to_poll_mask(int mask)
{
    short out = 0;
    if (mask & poll_data::EVENT_IN) out |= POLLIN;
    if (mask & poll_data::EVENT_OUT) out |= POLLOUT;
    if (mask & poll_data::EVENT_ERR) out |= POLLERR;
    return out;
}

int poll_create(poll_data & pdata, int max_con)
{
    pdata.max_con_ = max_con > FFPOLL_MAX_FDS ? FFPOLL_MAX_FDS : max_con;
    pdata.count_ = 0;
    pdata.result_num_ = 0;
    pdata.event_idx_ = 0;
    pdata.pollfds_ = new struct pollfd[pdata.max_con_];
    memset(pdata.pollfds_, 0, sizeof(struct pollfd) * pdata.max_con_);
    signal(SIGPIPE, SIG_IGN);
    return 0;
}

int poll_add(poll_data & pdata, int fd, int mask)
{
    if (pdata.count_ >= pdata.max_con_ || find_fd(pdata, fd) >= 0) return -1;
    int idx = pdata.count_++;
    pdata.fd_[idx] = fd;
    pdata.mask_[idx] = mask;
    pdata.pollfds_[idx].fd = fd;
    pdata.pollfds_[idx].events = to_poll_mask(mask);
    return 0;
}

int poll_mod(poll_data & pdata, int fd, int mask)
{
    int idx = find_fd(pdata, fd);
    if (idx < 0) return -1;
    pdata.mask_[idx] = mask;
    pdata.pollfds_[idx].events = to_poll_mask(mask);
    return 0;
}

int poll_del(poll_data & pdata, int fd)
{
    int idx = find_fd(pdata, fd);
    if (idx < 0) return -1;
    int last = --pdata.count_;
    if (idx != last)
    {
        pdata.fd_[idx] = pdata.fd_[last];
        pdata.mask_[idx] = pdata.mask_[last];
        pdata.pollfds_[idx] = pdata.pollfds_[last];
    }
    pdata.pollfds_[last].fd = -1;
    return 0;
}

int poll_wait(poll_data & pdata, int timeval)
{
    int ret = poll(pdata.pollfds_, (nfds_t)pdata.count_, timeval);
    pdata.result_num_ = 0;
    pdata.event_idx_ = 0;
    if (ret > 0)
    {
        for (int i = 0; i < pdata.count_; ++i)
        {
            short revents = pdata.pollfds_[i].revents;
            if (!revents) continue;
            int events = 0;
            if (revents & POLLIN) events |= poll_data::EVENT_IN;
            if (revents & POLLOUT) events |= poll_data::EVENT_OUT;
            if (revents & (POLLERR | POLLHUP | POLLNVAL)) events |= poll_data::EVENT_ERR;
            if (events)
            {
                pdata.result_fd_[pdata.result_num_] = pdata.fd_[i];
                pdata.result_event_[pdata.result_num_] = events;
                ++pdata.result_num_;
            }
        }
    }
    return ret;
}

int poll_destroy(poll_data & pdata)
{
    delete [] pdata.pollfds_;
    pdata.pollfds_ = NULL;
    return 0;
}

int poll_event(poll_data & pdata, int *fd, int *events)
{
    if (pdata.event_idx_ < pdata.result_num_)
    {
        *fd = pdata.result_fd_[pdata.event_idx_];
        *events = pdata.result_event_[pdata.event_idx_];
        ++pdata.event_idx_;
        return 0;
    }
    return -1;
}

#elif defined(ASERVER_WINDOWS)

static int find_fd(poll_data & pdata, int fd)
{
    for (int i = 0; i < pdata.count_; ++i)
    {
        if (pdata.fd_[i] == fd) return i;
    }
    return -1;
}

int poll_create(poll_data & pdata, int max_con)
{
    pdata.max_con_ = max_con > FFPOLL_MAX_FDS ? FFPOLL_MAX_FDS : max_con;
    pdata.count_ = 0;
    pdata.result_num_ = 0;
    pdata.event_idx_ = 0;
    return 0;
}

int poll_add(poll_data & pdata, int fd, int mask)
{
    if (pdata.count_ >= pdata.max_con_ || find_fd(pdata, fd) >= 0) return -1;
    int idx = pdata.count_++;
    pdata.fd_[idx] = fd;
    pdata.mask_[idx] = mask;
    return 0;
}

int poll_mod(poll_data & pdata, int fd, int mask)
{
    int idx = find_fd(pdata, fd);
    if (idx < 0) return -1;
    pdata.mask_[idx] = mask;
    return 0;
}

int poll_del(poll_data & pdata, int fd)
{
    int idx = find_fd(pdata, fd);
    if (idx < 0) return -1;
    int last = --pdata.count_;
    if (idx != last)
    {
        pdata.fd_[idx] = pdata.fd_[last];
        pdata.mask_[idx] = pdata.mask_[last];
    }
    return 0;
}

int poll_wait(poll_data & pdata, int timeval)
{
    FD_ZERO(&pdata.fdr_);
    FD_ZERO(&pdata.fdw_);
    FD_ZERO(&pdata.fde_);

    int max_fd = 0;
    for (int i = 0; i < pdata.count_; ++i)
    {
        SOCKET s = (SOCKET)pdata.fd_[i];
        if (pdata.mask_[i] & poll_data::EVENT_IN) FD_SET(s, &pdata.fdr_);
        if (pdata.mask_[i] & poll_data::EVENT_OUT) FD_SET(s, &pdata.fdw_);
        if (pdata.mask_[i] & poll_data::EVENT_ERR) FD_SET(s, &pdata.fde_);
        if (pdata.fd_[i] > max_fd) max_fd = pdata.fd_[i];
    }

    struct timeval tv;
    tv.tv_sec = timeval / 1000;
    tv.tv_usec = (timeval % 1000) * 1000;
    int ret = select(max_fd + 1, &pdata.fdr_, &pdata.fdw_, &pdata.fde_, &tv);

    pdata.result_num_ = 0;
    pdata.event_idx_ = 0;
    if (ret > 0)
    {
        for (int i = 0; i < pdata.count_; ++i)
        {
            SOCKET s = (SOCKET)pdata.fd_[i];
            int events = 0;
            if (FD_ISSET(s, &pdata.fdr_)) events |= poll_data::EVENT_IN;
            if (FD_ISSET(s, &pdata.fdw_)) events |= poll_data::EVENT_OUT;
            if (FD_ISSET(s, &pdata.fde_)) events |= poll_data::EVENT_ERR;
            if (events)
            {
                pdata.result_fd_[pdata.result_num_] = pdata.fd_[i];
                pdata.result_event_[pdata.result_num_] = events;
                ++pdata.result_num_;
            }
        }
    }
    return ret;
}

int poll_destroy(poll_data & pdata)
{
    return 0;
}

int poll_event(poll_data & pdata, int *fd, int *events)
{
    if (pdata.event_idx_ < pdata.result_num_)
    {
        *fd = pdata.result_fd_[pdata.event_idx_];
        *events = pdata.result_event_[pdata.event_idx_];
        ++pdata.event_idx_;
        return 0;
    }
    return -1;
}

#endif
