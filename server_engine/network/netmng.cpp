#include <sys/stat.h>
#ifdef __linux
#include <sys/resource.h>
#endif


#include "log.h"
#include "netmng.h"
#include "clientsocket.h"
#include "msg.h"
#ifdef __linux
#include "serversocket.h"
#endif

#include "lunar.h"

#define PACKET_SIZE_THRESHOLD (512)

namespace FF_Network {

void SockProcThread::run()
{
    while( active_ )
    {
        Buffer* buf = NULL;
        queue_->get_userdata( (void**)&buf );
        if( !buf ) return;
        int block_sz = sizeof(int) + sizeof(DPID);
        int event_id;
        memcpy( &event_id, buf->head_, sizeof(int) );
        buf->head_ += sizeof(int);
        while( buf->head_ < buf->tail_ )
        {
            int events;
            DPID dpid;
            memcpy( &events, buf->head_, sizeof(int) );
            memcpy( &dpid, buf->head_+sizeof(int), sizeof(DPID) );
            buf->head_ += block_sz;
#ifdef __linux
            parent_->multi_client_lock( true );
#endif
            CommonSocket * csock = parent_->get_sock(dpid);
#ifdef __linux
            parent_->multi_client_lock( false );
#endif
            if ( !csock )
            {
                ERR(2)("[NET](SockPorcThread) common sock dpid: %d null", dpid);
                continue;
            }
            int proc_result = csock->process(events, parent_);
            if ( proc_result < 0 )
            {
                /*!< 该连接已断开或发生了错误,从poll中清除掉.*/
                if ( parent_->get_select_thread()->fd_del(csock->Handle()) < 0 )
                {
                    ERR(2)("[NET](select_thread) SelectThread::run() fd_del() failed!");
                }
                /*!< 发送断开消息给上层处理.待上层来清除该socket而不直接从底层清除.*/
                parent_->post_disconnect_msg( csock->get_dpid(), 0, proc_result );
            }
        }
        parent_->sock_proc_event_->set_event(event_id);
    }
}

int g_network_frm = 0;

NetMng::NetMng()
{
	recv_buf_queue_		= BufferFactory::get_instance().create_bufque();
	send_buf_queue_		= BufferFactory::get_instance().create_bufque();
	svr_socket_			= NULL;
	started_			= false;
	pause_              = false;
    multi_client_       = false;
	more_limit_         = true;
	timeout_			= 0;
	timeout_list_head_	= NULL;
	timeout_list_tail_	= NULL;
	memset( socket_list_, 0, sizeof(socket_list_) ); /*!< 将socket指针队列置空*/
    proc_thread_num_    = 1;
	select_thread_.set_parent( this );
#ifdef DEBUG
	hook_func_			= NULL;
#endif
    memset( ip_, 0, sizeof(ip_) );

    INIT_LIST_HEAD( &recv_list_ );
    INIT_LIST_HEAD( &proc_list_ );

    pre_stop_           = false;
    sock_proc_thread_   = NULL;
    sock_proc_event_   = NULL;
    is_stop_recv_ = false;
    is_stop_send_ = false;
    server_id_ = -1;
    snapshot_type_ = 0x5007;
    is_buff_stats_open_ = true;
}

NetMng::~NetMng()
{
	stop_server();
}

/*********************************************************
	初始化函数,关闭函数
*********************************************************/

int NetMng::init()
{
	user_num_		= 0;
	del_usr_index_	= 0;

	if ( recv_buf_queue_ ) 
		recv_buf_queue_->clear( true );
	else
		recv_buf_queue_ = BufferFactory::get_instance().create_bufque();

	if ( send_buf_queue_ ) 
		send_buf_queue_->clear( true );
	else
		send_buf_queue_ = BufferFactory::get_instance().create_bufque();

	if ( !recv_buf_queue_ || !send_buf_queue_ )
	{
		ERR(2)("[NET](netmng) NetMng::NetMng() can not alloc memory of recv or send bufqueue.");
		exit(0);
	}

	return 0;
}

int NetMng::stop_server()
{
    // 1: stop select thread 
	select_thread_.stop();
    // 2: stop sock proc thread
    if( proc_thread_num_ > 1 && sock_proc_thread_ )
    {
        for(int i = 0; i<proc_thread_num_; i++)
        {
            sock_proc_thread_[i].post_sock_with_events( NULL );
            sock_proc_thread_[i].stop();
        }
        SAFE_DELETE_ARRAY( sock_proc_thread_ );
        SAFE_DELETE( sock_proc_event_ );
    }
#ifdef DEBUG
	if( hook_func_ )
	{
		hook_func_( HT_DISCONNECT, NULL );
	}
#endif
    // 3: destroy all
	destroy();
	return 0;
}

int NetMng::destroy()
{
	started_			= false;
	timeout_list_head_	= NULL;
	timeout_list_tail_	= NULL;

	SAFE_DELETE( recv_buf_queue_ );
	SAFE_DELETE( send_buf_queue_ );

	for (int i = 0; i < MAX_USER; ++i)
	{
		if ( NULL != socket_list_[i] )
		{
			SAFE_DELETE(socket_list_[i]);
		}
	}
	svr_socket_ = NULL; /*!< svr_socket_ 已经在前面socket_list_中被delete掉了,这里只需要置NULL*/
    reset_packet_stats();
	return 0;
}

/*! 其实现在只为关机用*/
void NetMng::set_pause( bool _flag )
{
	pause_ = _flag;
	if ( _flag ){
		select_thread_.stop();
	}
}

void NetMng::set_more_limit( bool _flag )
{
	more_limit_ = _flag;
}

void NetMng::reset_packet_stats()
{
    memset( send_buff_count_, 0, sizeof( send_buff_count_ ) ); 
    memset( send_buff_size_, 0, sizeof( send_buff_size_ ) ); 
    memset( recv_buff_count_, 0, sizeof( recv_buff_count_ ) ); 
    memset( recv_buff_size_, 0, sizeof( recv_buff_size_ ) ); 
}

void NetMng::set_packet_stats_open( bool _is_open, ulong _snapshot_type )
{
    is_buff_stats_open_ = _is_open;
    snapshot_type_ = _snapshot_type;
}

void NetMng::print_packet_stats()
{
    if( !is_buff_stats_open_ )
    {
        return;
    }

    AutoLock lock(&buff_stats_mutex_);

    ulong total_count = 0; 
    ulong total_size = 0; 

    PROF(2)("[RECV_BUFF_STATS] ------ begin -------" );
    for( int i=0; i<0xFFFF; i++ ) {
        if ( recv_buff_count_[i]>0 ) {
            total_count += recv_buff_count_[i];
            total_size += recv_buff_size_[i];
            TRACE(1)("[RECV_BUFF_LOG] packet_type: 0x%04x, packet_count: %u\t\t\t, total_size: %u\t\t\t, average_size: %u", i, recv_buff_count_[i], recv_buff_size_[i], recv_buff_size_[i]/recv_buff_count_[i] );
        }    
    }    
    PROF(2)("[RECV_BUFF_STATS] ------ end, total_count: %u, total_size: %u -------", total_count, total_size );

    total_count = 0; 
    total_size = 0; 

    PROF(2)("[SEND_BUFF_STATS] ------ begin -------" );
    for( int i=0; i<0xFFFF; i++ ) {
        if ( send_buff_count_[i]>0 ) {
            total_count += send_buff_count_[i];
            total_size += send_buff_size_[i];
            TRACE(1)("[SEND_BUFF_LOG] packet_type: 0x%04x, packet_count: %u\t\t\t, total_size: %u\t\t\t, average_size: %u", i, send_buff_count_[i], send_buff_size_[i], send_buff_size_[i]/send_buff_count_[i] );
        }    
    }    
    PROF(2)("[SEND_BUFF_STATS] ------ end, total_count: %u, total_size: %u -------", total_count, total_size );

    reset_packet_stats();
}

void NetMng::mark_packet_stats( const char* _ptr, bool _is_recv )
{
    if( !is_buff_stats_open_ )
    {
        return;
    }

    ff_header_t data_size = *(ff_header_t*)_ptr;
	packet_type_t packet_type = *(packet_type_t*)(_ptr + HEADER_SIZE);

    AutoLock lock(&buff_stats_mutex_);

    if( _is_recv )
    {
        recv_buff_count_[packet_type] += 1;
        recv_buff_size_[packet_type] += data_size;
    }
    else
    {
        if( packet_type != snapshot_type_ )
        {
            send_buff_count_[packet_type] += 1;
            send_buff_size_[packet_type] += data_size;
            if( data_size > PACKET_SIZE_THRESHOLD && packet_type != ST_RECONNECT )
            {
                PROF(1)("[NetMng](mark_packet_stats) large packet, packet_type: 0x%04x, packet_size: %u", packet_type, data_size );
            }
        }
    }
}

void NetMng::mark_snapshot_stats( packet_type_t _packet_type, int _data_size )
{
    AutoLock lock(&buff_stats_mutex_);
    send_buff_count_[_packet_type] += 1;
    send_buff_size_[_packet_type] += _data_size;
    if( _data_size > PACKET_SIZE_THRESHOLD && _packet_type != ST_ADD_OBJ )
    {
        PROF(1)("[NetMng](mark_snapshot_stats) large packet, packet_type: 0x%04x, packet_size: %u", _packet_type, _data_size );
    }
}

/*********************************************************
	添加删除套接字函数
*********************************************************/

CommonSocket * NetMng::get_sock_fd(int fd) const
{
#ifdef _WIN32
	return socket_list_[0];
#else
	if ( ( fd < 0 ) || ( fd >= MAX_USER) )
	{
		return NULL;	
	}
	return socket_list_[fd];
#endif
}

CommonSocket * NetMng::get_sock(DPID dpid) const
{
	CommonSocket * sock =  get_sock_fd( dpid & 0x0000FFFF );
	if ( sock && ( dpid != sock->get_dpid() ) )
	{
			return NULL;
	}
	return sock;
}

int NetMng::add_sock(CommonSocket * sock, bool time_out)
{
#ifdef _WIN32
	int fd = 0;
#else
	int fd = sock->get_dpid() & 0x0000FFFF;
#endif
	
	if ( ( fd < 0 ) || ( fd >= MAX_USER ) )
	{
		return -1;
	}

	if ( socket_list_[fd] != NULL )
	{
		return -1;
	}

	socket_list_[fd] = sock;
	user_num_++;

	if( time_out )
	{
		timeout_addtail(sock);
	}

	return 0;
}

int NetMng::del_sock(DPID dpid)
{
	CommonSocket * csock = get_sock(dpid);
	if (!csock)
	{
		return -1;
	}
	/*!< 将要删除的socket的dpid放入删除队列中,等待网络线程来读取并删除*/
	AutoLock lock(&del_usr_mutex_);
	if ( del_usr_index_ < MAX_USER )
		del_usr_list_[del_usr_index_++] = dpid;
	return 0;
}

/*!< 网络线程调用该函数从删除队列中读取dpid,按照dpid删除对应socket*/
int NetMng::do_del_sock()
{
	AutoLock lock(&del_usr_mutex_);
	for (int i = 0; i < del_usr_index_; ++i)
	{
		DPID dpid = del_usr_list_[i];
		int fd = dpid & 0x0000FFFF;
		
		CommonSocket * csock = get_sock(dpid);
		if (!csock)
		{
//			LOG(2)("NetMng::do_del_sock() commonsocket null! dpid : %d, fd : %d", dpid, fd);
			continue;
		}
//LOG(2)("del_sock()  fd : %d", fd);
		socket_list_[fd] = NULL;
		user_num_--;
		timeout_unlink(csock);
		SAFE_DELETE(csock);
	}
	del_usr_index_ = 0;
	return 0;
}

/*********************************************************
	处理超时函数
*********************************************************/

/*! 用于将一个CommonSocket添加到netmng的超时链表的尾部*/
void NetMng::timeout_addtail(CommonSocket * csock)
{
    AutoLock lock( &timeout_mutex_);
	assert(csock);

	if(!timeout_list_head_)
		timeout_list_head_ = csock;
	if(!timeout_list_tail_)
		timeout_list_tail_ = csock;
	else
	{
		timeout_list_tail_->timeout_next_ = csock;
		csock->timeout_prev_ = timeout_list_tail_;
		timeout_list_tail_ = csock;
	}
}

/*! 用于把一个CommonSocket从netmng的超时链表之中移除*/
void NetMng::timeout_unlink(CommonSocket * csock)
{
    AutoLock lock( &timeout_mutex_);
	assert(csock);

	if(timeout_list_head_ == csock)
		timeout_list_head_ = csock->timeout_next_;
	if(timeout_list_tail_ == csock)
		timeout_list_tail_ = csock->timeout_prev_;
	if(csock->timeout_next_)
	{
		csock->timeout_next_->timeout_prev_ = csock->timeout_prev_;
	}
	if(csock->timeout_prev_)
	{
		csock->timeout_prev_->timeout_next_ = csock->timeout_next_;
	}
	csock->timeout_next_ = NULL;
	csock->timeout_prev_ = NULL;
}

/*当某个socket收到了消息的时候，将其从超时队列中原来的位置移动至超时队列的尾部。
确保没收到消息越久的连接排在超时队列的越前面。
*/
void NetMng::refresh_timeout(CommonSocket * csock)
{
	assert(csock);

	csock->timeout_ = g_network_frm;
	if(timeout_list_tail_ == csock)
		return;
	
	timeout_unlink(csock);
	timeout_addtail(csock);
}

/*! 从超时队列的头部开始检测CommonSocket是否超时，由于头部的CommonSocket都
是没有收到消息时间最长的CommonSocket，所以只需要从头部开始顺序检测超时就可
以了。只要头部的没有超时，就可以保证后面的所有CommonSocket都是没有超时的。
*/
void NetMng::check_timeout()
{
	if( 0 == timeout_ )
	{
		return;
	}

	while(timeout_list_head_)
	{
		CommonSocket * csock = timeout_list_head_;
		if(g_network_frm - csock->timeout_ > timeout_)
		{
			/*!< 该连接已超时,从poll中清除掉.*/
			if ( select_thread_.fd_del(csock->Handle()) < 0 )
			{
				LOG(2)("[NET](netmng) NetMng::check_timeout() fd_del() failed! sock:0x%08x, dpid:0x%08X, ErrCode:%d", 
                        csock->Handle(), csock->get_dpid(), csock->ErrNo());
			}
			/*!< 发送断开消息给上层处理.待上层来清除该socket而不直接从底层清除.*/
			post_disconnect_msg( csock->get_dpid(), 0, 1 );
            in_addr_t ip = csock->get_real_ip();
			LOG(2)("[NET](netmng) NetMng::check_timeout() sock:0x%08x, dpid: 0x%08X, ip:%d.%d.%d.%d, network_frm: %u, sock_timeout: %u, sys_timeout: %d", csock->Handle(), csock->get_dpid(), ip&0x000000ff, (ip&0x0000ff00) >> 8, (ip&0x00ff0000) >> 16,(ip&0xff000000) >> 24, g_network_frm, csock->timeout_, timeout_ );

			timeout_unlink(csock);
		}
		else
			return;
	}
}

/*********************************************************
	监听函数,连接函数
*********************************************************/

int NetMng::init_connection(int port, const char * addr)
{
	remote_addr_.Port(port);
	if (addr)
		remote_addr_.Address(addr);
	else
		remote_addr_.Address((unsigned long)INADDR_ANY); /*!< 为服务端时默认监听所有地址*/
	remote_addr_.Family(AF_INET);
	return 0;
}

#ifdef __linux

/*! 一般服务器所要求的可连接数量都会大于一般linux系统所设置的最大打开文件数,
	所以如果是服务器端启动监听端口,则需要检测并设置最大打开文件数 */
int set_nofile(u_long nofile)
{
	struct rlimit rlim_nofile;

	if ( getrlimit(RLIMIT_NOFILE, &rlim_nofile) < 0 )
	{
		return -1;
	}

	if ( rlim_nofile.rlim_cur < nofile )
	{
		rlim_nofile.rlim_cur = nofile;	/* Soft limit */
		rlim_nofile.rlim_max = nofile;	/* Hard limit (ceiling
	                                      for rlim_cur) */
		return setrlimit(RLIMIT_NOFILE, &rlim_nofile);
	}

	return 0;
}

int NetMng::start_server( int max_con, int port, bool crypt, const char* ip, int timeval, int pthread_num )
{
	assert(max_con > 0);

	stop_server(); /*!< 先关闭之前的服务*/
	init(); /*!< 初始化服务*/

	server_mng_ = true;
	need_crypt_ = crypt;
	timeval_ = timeval;
    multi_client_ = false;
	max_fd_ = max_con << 2; /*!< 网络底层所能容纳的连接数要远远高于逻辑层传入的最大用户数量,
								以防止在Dos攻击下彻底瘫痪.这里将可容纳的连接数设为最大用户数量的四倍.
							*/
    proc_thread_num_ = pthread_num;
    if( proc_thread_num_ > 1 )
    {
        sock_proc_thread_ = new SockProcThread[proc_thread_num_];
        for( int i = 0; i < proc_thread_num_; i++ )
        {
            sock_proc_thread_[i].set_parent( this );
            sock_proc_thread_[i].start();
        }
        sock_proc_event_ = new Event( proc_thread_num_ );
    }
    
	if ( max_fd_ > MAX_USER )
	{
		ERR(2)("[NET](netmng) NetMng::start_server() set too large max_connection! max_limit = %d", (MAX_USER >> 2) );
		return -1;
	}

	// 为测试时方便, 临时关闭 sodme 20060608
	if ( set_nofile(max_fd_) < 0 )
	{
		ERR(2)("[NET](netmng) NetMng::start_server() can not set nofile to %d! [%s]", max_fd_, strerror(errno));
		return -1;
	}

	init_connection(port, ip);/*!< 初始化服务地址*/

	if ( create_session() < 0 )
	{
		destroy();
		ERR(2)("[NET](netmng) NetMng::start_server() create_session() failed!");
		return -1;
	}
	if ( select_thread_.start() < 0)
	{
		destroy();
		ERR(2)("[NET](netmng) NetMng::start_server() start select thread failed!");
		return -1;
	}

	sleep( 3 );

	LOG(2)("[NET](netmng) NetMng::start_server(), set_nofile, max_fd: %d", max_fd_);
	started_ = true;

	return 0;
}

int NetMng::create_session()
{
	svr_socket_ = new ServerSocket( need_crypt_ );

	if ( !svr_socket_ )
	{
		ERR(2)("[NET](netmng) NetMng::create_session() can not alloc memory of svr_socket_.");
		exit(0);
	}

	if ( svr_socket_->do_listen(remote_addr_, 50) < 0 )
	{
		ERR(2)("[NET](netmng) NetMng::create_session() listen failed on %s", remote_addr_.GetString() );
		return -1;
	}

	LOG(2)("[NET](netmng) listen on %s! server fd: %d", remote_addr_.GetString() , svr_socket_->Handle());

	this->add_sock( svr_socket_, false );
	return 0;
}

#endif


int NetMng::connect_server(const char * addr, int port, bool crypt, int timeval)
{
	stop_server(); /*!< 先关闭之前的服务*/
	init();

	server_mng_ = false;
    multi_client_ = false;
	need_crypt_ = crypt;
	timeval_ = timeval;
	max_fd_ = 1;

	init_connection(port, addr); /*!< 初始化服务地址*/

	if ( join_session() < 0 )
	{
		destroy();
		ERR(2)("[NET](netmng) NetMng::connect_server() join_session() failed!");
		return -1;
	}

	if ( select_thread_.start() < 0 )
	{
		destroy();
		ERR(2)("[NET](netmng) NetMng::connect_server() start select thread failed!");
		return -1;
	}

	started_ = true;
#ifdef DEBUG
	if( hook_func_ )
	{
		hook_func_( HT_CONNECT, &remote_addr_ );
	}
#endif
	return 0;
}

#ifdef __linux

int NetMng::init_multi_client( int _max_con, int _timeval )
{
    if( _max_con > MAX_USER ) {
        ERR(2)( "[NET](netmng) init_multi_client, invalid max_connection! max_limit = %d", MAX_USER );
        return -1;
    }

    stop_server();
    init();
    server_mng_     = false;
    multi_client_   = true;
    timeval_        = _timeval;
    max_fd_         = _max_con;

    if( select_thread_.start() < 0 ) {
        destroy();
        ERR(2)( "[NET](netmng) init_multi_client, start select thread failed" );
        return -1;
    }

    started_ = true;

    LOG(2)( "[NET](netmng) init_multi_client, max_con: %d", max_fd_ );

    return 0;
}

ClientSocket* NetMng::connect_server_new( const char* _ip, int _port, bool _crypt )
{
    if( !is_multi_client() ) {
        ERR(2)( "[NET](netmng) connect_server_new, not multi client" );
        return 0;
    }
    if( user_num_ >= max_fd_ ) {
        ERR(2)( "[NET](netmng) connect_server_new, use reaches max: %d", max_fd_ );
        return 0;
    }

    ClientSocket *sock = new ClientSocket( _crypt );
    if( !sock ) {
        ERR(2)( "[NET](netmng) connect_server_new, can not alloc memory for ClientSocket" );
        return 0;
    }

    EndPoint ep( _ip, _port, AF_INET );
    if( sock->do_connect( ep ) < 0 ) {
        delete sock;
        ERR(2)( "[NET](netmng) connect_server_new, can not connect to server %s:%d", _ip, _port );
        return 0;
    }

    AutoLock lock(&multi_client_mutex_);
    if( select_thread_.fd_add( sock->Handle(), poll_data::EVENT_IN ) < 0 ) {
        delete sock;
        ERR(2)( "[NET](netmng) connect_server_new, add to select thread failed" );
        return 0;
    }

    if( add_sock( sock ) < 0 ) {
        delete sock;
        ERR(2)( "[NET](netmng) connect_server_new, add to netmng failed" );
        return 0;
    }

    sock->set_peer( ep );
    post_connect_msg( sock );

    LOG(2)( "[NET](netmng) create new connection to server %s:%d! dpid: %d", _ip, _port, sock->get_dpid() );

    return sock;
}

void NetMng::multi_client_lock( bool _lock )
{
    if( !is_multi_client() ) return;

    if( _lock )
        multi_client_mutex_.Lock();
    else
        multi_client_mutex_.Unlock();
}

#endif // _linux

int NetMng::join_session()
{
	svr_socket_ = new ClientSocket( need_crypt_ );

	if ( !svr_socket_ )
	{
		ERR(2)("[NET](netmng) NetMng::join_session() can not alloc memory of svr_socket_.");
		exit(0);
	}
/* 对windows客户端采用连接线程的形式在selectthread中进行connect,防止主线程阻塞,而在linux下则直接阻塞连接*/
	this->add_sock( svr_socket_, false );
#ifdef __linux
	if ( svr_socket_->do_connect(remote_addr_) < 0 )
	{
		ERR(2)("[NET](netmng) NetMng::join_session() can not connect to server %s", remote_addr_.GetString() );
		return -1;
	}
	post_connect_msg( svr_socket_ );
	LOG(0)("[NET](netmng) connect to server %s successful! server fd: %d", remote_addr_.GetString() , svr_socket_->Handle());
#endif
	return 0;
}

/*********************************************************
	发送接受消息函数
*********************************************************/

BufferQueue * NetMng::get_recv_msg()
{
	if (!started_) return NULL;

	assert(recv_buf_queue_);

	AutoLock lock(&recv_bufq_mutex_);
    if( list_empty( &(recv_buf_queue_->link_) ) ) return NULL;
	BufferQueue * _old_queue = recv_buf_queue_;
	recv_buf_queue_ = BufferFactory::get_instance().create_bufque();
	if ( !recv_buf_queue_ )
	{
		ERR(2)("[NET](netmng) NetMng::get_recv_msg() can not alloc memory of recv buf queue.");
		exit(0);
	}
	return _old_queue;
}

int NetMng::add_recv_msg(Buffer * buf)
{
    recv_lock_.Lock();
    list_add_tail( &buf->link_, &recv_list_ );
    recv_lock_.Unlock();
    CommonSocket::recv_up();
	return 0;
}


BufferQueue * NetMng::get_send_msg()
{
    if( !started_) return NULL;
    AutoLock lock( &send_bufq_mutex_);
    if( list_empty( &(send_buf_queue_->link_) ) ) return NULL;

    BufferQueue * _old_queue = send_buf_queue_;
	send_buf_queue_ = BufferFactory::get_instance().create_bufque();
	if ( !send_buf_queue_ ) {
		ERR(2)("[NET](netmng) NetMng::get_send_msg() can not alloc memory of send buf queue.");
		exit(0);
	}
	return _old_queue;
}

int NetMng::add_send_msg(Buffer * buf)
{
	if (!started_ || pre_stop_ )
	{
		SAFE_DELETE(buf);
		return -1;
	}

	assert(buf);
	assert(send_buf_queue_);

	if( buf->dpid_ == DPID_SERVER_PLAYER )
	{
		buf->dpid_ = get_svr_sock()->get_dpid();
	}
#ifdef DEBUG
	if( hook_func_ )
	{
		hook_func_( HT_BUFFER, buf );
	}
#endif
	AutoLock lock(&send_bufq_mutex_);
	send_buf_queue_->add_tail(buf);
    CommonSocket::send_up();
    mark_packet_stats(buf->buf_start_, false);
	return 0;
}

int NetMng::send_msg(const char * buf, size_t len, DPID dpid )
{
	if (!started_) return -1;

	if ( !buf || !len )
		return 0;

	if ( pause_ ) {
		return 0;
	}

    Buffer* buffer = BufferFactory::get_instance().create_buffer( len + HEADER_SIZE );
    if( buffer == NULL ) {
		ERR(2)("[NET](netmng) NetMng::send_msg(), BUFFER_ERROR, packet_size = 0x%08X, dpid = 0x%08X", len + HEADER_SIZE, dpid );
        return 0;
    }
    buffer->set_header( len );
	memcpy( buffer->tail_, buf, len );
	buffer->tail_ += len;

    if( dpid == DPID_SERVER_PLAYER ) {
        buffer->dpid_ = get_svr_sock()->get_dpid();
    } else {
        buffer->dpid_ = dpid;
    }

	this->add_send_msg(buffer);
    return 0;
}

void NetMng::receive_msg()
{
	if ( pause_ || pre_stop_ ) {
		return;
	}

    static unsigned long long tick_per_sec = get_cpu_tick();
    recv_lock_.Lock();
    list_splice_tail( &recv_list_, &proc_list_ );
    INIT_LIST_HEAD( &recv_list_ );
    recv_lock_.Unlock();

    static int recv_count = 0;
    static int send_count = 0;

    int rc = CommonSocket::get_recv_num();
    int sc = CommonSocket::get_send_num();
    int bc = Buffer::count_;

    //TRACE(0)( "LOG [BUFFER_COUNT] recv = %d, send = %d, count = %d", rc, sc, bc );     

    bool blog = false;
    if( recv_count < rc ) {
        recv_count = rc; 
        blog = true;
    }

    if( send_count < sc ) {
        send_count = sc;
        blog = true;
    }


    if( blog ) {
        PROF(0)( "[BUFFER_COUNT] recv = %d, send = %d, count = %d", rc, sc, bc );     
    }

    TICK(A)
    int count = 0;
    while( !list_empty( &proc_list_) ) {
        TICK(B)
        if( ( more_limit_ ) && ( B - A > tick_per_sec * 0.2 )) {
            ERR(0)( "[NETMNG] too_more_packet, count = %d", count);
            break;
        }

        list_head* pos = proc_list_.next;
        list_del( pos );
        Buffer* buffer = list_entry( pos, Buffer, link_ );
        char* ptr = buffer->buf_start_;
        while( buffer->cb_-- > 0 ) {
            int data_size = buffer->get_packet_size( ptr );
            if( data_size >= (int)(HEADER_SIZE + PACKET_TYPE_SIZE ) ) {
				packet_type_t packet_type = *(packet_type_t*)(ptr + HEADER_SIZE);
				msg_handle( ptr + HEADER_SIZE + PACKET_TYPE_SIZE, data_size - HEADER_SIZE - PACKET_TYPE_SIZE, packet_type, buffer->dpid_ );
                mark_packet_stats( ptr, true );
            }
            ptr += data_size;
            count++;
        }
        CommonSocket::recv_down();
        SAFE_DELETE( buffer );
    }
}

int NetMng::post_disconnect_msg( DPID _dpid, int now_disconn, int from )
{
    assert(_dpid != DPID_UNKNOWN );

    Buffer * dis_buf = BufferFactory::get_instance().create_buffer();
    if (!dis_buf) {
        ERR(2)("[NET](netmng) NetMng::post_disconnect_msg() can not alloc memory of disconnect msg buf.");
        return -1;
    }
    packet_type_t dis_type = PT_DISCONNECT;
    dis_buf->set_header( PACKET_TYPE_SIZE + sizeof(int) );
    memcpy( dis_buf->tail_, (char*)&dis_type, PACKET_TYPE_SIZE );
    dis_buf->tail_ += PACKET_TYPE_SIZE;
    *(int*)(dis_buf->tail_) = (int)now_disconn;
    dis_buf->tail_ += sizeof(int);
    dis_buf->cb_++;
    dis_buf->dpid_ = _dpid; 
    add_recv_msg(dis_buf);
    
	//LOG(2)("[NET](netmng) NetMng::post_disconnect_msg() dpid = 0x%08X, now_disconn: %d, from: %d", _dpid, now_disconn, from );

    return 0;

}

int NetMng::post_connect_msg(CommonSocket * csock)
{
	assert(csock);

	Buffer * dis_buf = BufferFactory::get_instance().create_buffer();
	if (!dis_buf)
	{
		ERR(2)("[NET](netmng) NetMng::post_connect_msg() can not alloc memory of connect msg buf.");
		return -1;
	}
	packet_type_t dis_type = PT_CONNECT;
	dis_buf->set_header( PACKET_TYPE_SIZE );
	memcpy( dis_buf->tail_, (char*)&dis_type, PACKET_TYPE_SIZE );
	dis_buf->tail_ += PACKET_TYPE_SIZE;
	dis_buf->cb_++;
	dis_buf->dpid_ = csock->get_dpid();
	add_recv_msg(dis_buf);
	return 0;
}

int initial_startup()
{
    int signature = luaF_getopcode_signature();
    apply_signature( signature );
    return 0;
}

void final_cleanup()
{
#ifndef __linux
	unref_encode_func();
#endif
}

#include <net/if.h>
char* NetMng::get_ip()
{
    if( ip_[0] != '\0'  )
        return ip_;

    int sock;
    struct sockaddr_in sin;
    //struct sockaddr sa;
    struct ifreq ifr;
    //unsigned char mac[6];

    sock = ff_socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == -1) {
        perror("Error: get local IP socket fail!");
        return NULL;
    }

    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        perror("Error: get local IP ioctl fail!");
        return NULL;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    sprintf( ip_, "%s",inet_ntoa(sin.sin_addr));

    ff_close( sock );

    return ip_;
}

} //namespace FF_Network
