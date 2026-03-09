#include "misc.h"
#include "log.h"
#include "selectthread.h"
#include "netmng.h"
//#include "msghdr.h"

namespace FF_Network {

SelectThread::SelectThread()
{
    thread_name_ = "SelectThread";
	parent_ = NULL;
    buf_array_ = NULL;
    buf_array_count_ = 0;
    memset( &pdata_, 0, sizeof(pdata_) );
}

SelectThread::~SelectThread()
{
    delete_buf_array();
}

void SelectThread::new_buf_array()
{
    if( buf_array_count_ > 1 ) 
    {
        delete_buf_array();

        int block_sz = sizeof(int) + sizeof(DPID);
        buf_array_ = new Buffer*[ buf_array_count_ ];
        for( int i = 0; i < buf_array_count_; i++)
        {
            buf_array_[i] = new Buffer( (MAX_USER+1) * block_sz );
            memcpy( buf_array_[i]->tail_, &i, sizeof(int) );
            buf_array_[i]->tail_ += sizeof(int);
        }
    }

}

void SelectThread::delete_buf_array()
{
    if (buf_array_)
    {
        for( int i = 0; i < buf_array_count_; i++)
        {
            SAFE_DELETE(buf_array_[i])
        }
        SAFE_DELETE_ARRAY( buf_array_ );
    }

}

void SelectThread::set_parent(NetMng * parent)
{
	parent_ = parent;
    buf_array_count_ = parent_->proc_thread_num_;
    new_buf_array();
}

int SelectThread::dispense_msg()
{
    assert( parent_ );
	BufferQueue * send_bufque = parent_->get_send_msg();
	if( send_bufque ) {
        while( !list_empty( &(send_bufque->link_ ) ) ) {
            list_head* pos = send_bufque->link_.next;
            list_del( pos );
            Buffer* buf = list_entry( pos, Buffer, link_ );
            CommonSocket * csock = parent_->get_sock(buf->dpid_);
			if ( csock && !fd_mod( csock->Handle(), poll_data::EVENT_IN | poll_data::EVENT_OUT ) ) {
				csock->send_msg(buf);
			} else {
				SAFE_DELETE(buf);
                CommonSocket::send_down();
			}
        }     
		SAFE_DELETE(send_bufque);
    }
    return 0;
}

void SelectThread::run()
{
	assert(parent_);

/* windows下在独立线程中来进行connect操作，防止主界面锁死*/
#ifndef __linux
	if ( !parent_->is_server() )
	{
		CommonSocket* svr_socket = parent_->get_svr_sock();
        if( svr_socket ) {
            const EndPoint* ep = parent_->get_remote_addr();
            assert( ep );
            if ( svr_socket->do_connect( *ep ) >= 0 )
            {
                parent_->post_connect_msg( svr_socket );
            }
            else
            {
                parent_->post_disconnect_msg( svr_socket->get_dpid(), 0, 2 );
                return;
            }
        }
	}
	else
	{
		assert(0);
	}
#endif

	/*!< 创建poll对象*/
	if ( poll_create(pdata_, parent_->get_max_fd()) < 0 )
	{
		ERR(2)("[NET](select_thread) SelectThread::run() poll create failed! code : %d", parent_->get_svr_sock()->ErrNo() );
		exit(0);
	}

	/*!< 添加服务端口至poll*/
    if( !parent_->is_multi_client() ) { 
        if ( fd_add( parent_->get_svr_sock()->Handle() ) < 0 )
        {
            ERR(2)("[NET](select_thread) SelectThread::run() can not add server sock to poll!");
            ERR(2)( "[NET](select_thread) parent_->get_svr_sock()->Handle() %d, errno %d\n", parent_->get_svr_sock()->Handle(), errno );
            exit(0);
        }
    }

    if (buf_array_count_ != parent_->proc_thread_num_){
        buf_array_count_ = parent_->proc_thread_num_;
        new_buf_array();
    }

	while ( active_ )
    {
        if ( poll_wait(pdata_, parent_->get_timeval()) > 0 )
        {
            int fd, events;
            while ( !poll_event(pdata_, &fd, &events) )
            {
#ifdef __linux
                parent_->multi_client_lock( true );
#endif
                CommonSocket * csock = parent_->get_sock_fd(fd);
#ifdef __linux
                parent_->multi_client_lock( false );
#endif
                if ( !csock )
                {
                    ERR(2)("[NET](select_thread) SelectThread::run() common sock null");
                    continue;
                }

                if( parent_->proc_thread_num_ <= 1 || csock == parent_->get_svr_sock() )
                {
                    int proc_result = csock->process(events, parent_);
                    if ( proc_result < 0 )
                    {
                        /*!< 该连接已断开或发生了错误,从poll中清除掉.*/
                        if ( fd_del(fd) < 0 )
                        {
                            ERR(2)("[NET](select_thread) SelectThread::run() fd_del() failed!");
                        }
                        /*!< 发送断开消息给上层处理.待上层来清除该socket而不直接从底层清除.*/
                        parent_->post_disconnect_msg( csock->get_dpid(), 0, proc_result );
                    }
                }
                else
                {
                    int block_sz = sizeof(int) + sizeof(DPID);
                    DPID dpid = csock->get_dpid();
                    int index = ( dpid >> 16 ) % parent_->proc_thread_num_; 

                    Buffer* buf = buf_array_[index];
                    if( buf->tail_ < buf->buf_max_ )
                    {
                        memcpy( buf->tail_, &events, sizeof(int) );
                        memcpy( buf->tail_+sizeof(int), &dpid, sizeof(DPID) );
                        buf->tail_ += block_sz;
                    }
                    else
                    {
                        ERR(2)( "[NET](SelectThread)buf size reach max. start: %p, tail: %p, max: %p", buf->buf_start_, buf->tail_, buf->buf_max_ );
                    }
                }
            }
            if( parent_->proc_thread_num_ > 1 )
            {
                for( int i = 0; i < parent_->proc_thread_num_; i++ )
                {
                    if( buf_array_[i] )
                    {
                        parent_->sock_proc_thread_[i].post_sock_with_events( buf_array_[i] );
                    }
                }
                for( int i = 0; i < parent_->proc_thread_num_; i++ )
                {
                    if( buf_array_[i] )
                    {
                        parent_->sock_proc_event_->wait_event(i);
                        buf_array_[i]->head_ = buf_array_[i]->buf_start_;
                        buf_array_[i]->tail_ = buf_array_[i]->head_ + sizeof(int); 
                    }
                }
            }
        }
#ifdef __linux
        parent_->multi_client_lock( true );
#endif
		parent_->check_timeout();
		/*!< 删除用户*/
		parent_->do_del_sock();
		dispense_msg();
		//ff_sleep(parent_->get_timeval());
#ifdef __linux
        parent_->multi_client_lock( false );
#endif
	}
	parent_->do_del_sock();
	poll_destroy(pdata_);
}

int SelectThread::fd_add(int fd, int mask)
{
	return poll_add(pdata_, fd, mask);
}

int SelectThread::fd_mod(int fd, int mask)
{
	return poll_mod(pdata_, fd, mask);
}

int SelectThread::fd_del(int fd)
{
	return poll_del(pdata_, fd);
}

} //namespace FF_Network
