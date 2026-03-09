#ifndef __SELECT_THREAD_H__
#define __SELECT_THREAD_H__

#include "thread.h"
#include "ffpoll.h"

class Buffer;

namespace FF_Network {

class NetMng;
class CommonSocket;

class SelectThread : public Thread
{
public:
	SelectThread();
	~SelectThread();

	void set_parent(NetMng * parent);
	void run();

	/*!< 提供套接字状态设置接口*/
	int fd_add(int fd, int mask = poll_data::EVENT_IN );
	int fd_mod(int fd, int mask);
	int fd_del(int fd);
private:
    void new_buf_array();
    void delete_buf_array();

	poll_data pdata_;
    Buffer** buf_array_;
    int buf_array_count_;
	NetMng * parent_;

	int dispense_msg();
};

}

#endif
