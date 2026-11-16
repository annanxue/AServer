#include "event.h"

Event::Event(int size) : size_(size)
{
    if (size_ > MAX_EVENTS) size_ = MAX_EVENTS;
    if (size_ < 1) size_ = 1;
    memset(events_, 0, sizeof(events_));
    FF_MUTEX_INIT(&mutex_);
    FF_COND_INIT(&cond_);
}

Event::~Event()
{
    FF_COND_DESTROY(&cond_);
    FF_MUTEX_DESTROY(&mutex_);
}

void Event::set_event(int idx)
{
    if (idx < 0 || idx >= size_) return;
    FF_MUTEX_LOCK(&mutex_);
    events_[idx] = 1;
    FF_COND_BROADCAST(&cond_);
    FF_MUTEX_UNLOCK(&mutex_);
}

void Event::wait_event(int idx)
{
    if (idx < 0 || idx >= size_) return;
    FF_MUTEX_LOCK(&mutex_);
    while (events_[idx] == 0)
    {
        FF_COND_WAIT(&cond_, &mutex_);
    }
    events_[idx] = 0;
    FF_MUTEX_UNLOCK(&mutex_);
}

void Event::wait_all_event()
{
    for (int i = 0; i < size_; ++i)
    {
        wait_event(i);
    }
}
