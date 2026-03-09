#ifndef __THREAD_H__
#define __THREAD_H__

#include "ffsys.h"

class Thread
{
public:
	Thread();
	virtual ~Thread();
	void		join();
	int			start();
	int			stop();
    
protected:
    const char* thread_name_;
	pthread_t	thread_id_;
	int			active_; /* run()函数内部用于结束线程的标志*/

	friend void	thread_start(void *p);
	virtual void run() = 0;
};

#endif

