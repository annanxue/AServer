#ifndef __NETMNG_H__
#define __NETMNG_H__

#include "msgtype.h"
#include "autolock.h"
#include "endpoint.h"
#include "commonsocket.h"
#include "selectthread.h"
#include "cpcqueue.h"
#include "misc.h"

#ifdef _WIN32
#define MAX_USER		1
#else
#define MAX_USER		32768
#endif

#ifndef __linux
#include "pike_lua.h"
#endif

#define NEW_RECV_LIST
namespace FF_Network {

class ClientSocket;

class SockProcThread : public Thread
{
public:
    SockProcThread() { thread_name_ = "SockProThread";  queue_ = new PcQueue(8); }
    ~SockProcThread() { SAFE_DELETE( queue_ ); }
    void run();
    void set_parent( NetMng* _parent ) { parent_ = _parent; }
    void post_sock_with_events( Buffer* _buf ) { queue_->post_userdata( (void*)_buf ); }
private:
    PcQueue* queue_;
    NetMng* parent_;
};

class NetMng
{
public:	
	NetMng();
	virtual ~NetMng();
    
	int				init();
	int				destroy();

	/*!< 非linux系统下无法启动为服务端*/
#ifdef __linux
	int             start_server(int max_con, int port, bool crypt = true,const char* ip = NULL, int timeval = 100, int pthread_num = 1);
	int				create_session();	/* 启动端口监听 */
#endif

	int				connect_server(const char * addr, int port, bool crypt = true, int timeval = 100);
	int				stop_server();
       				
	int				init_connection(int port, const char * addr = NULL);	/* 初始化服务器地址*/
	
	int				join_session();		/* 连接目标服务器 */
#ifdef __linux
    // 非linux系统不能使用多客户端模式
    int             init_multi_client( int _max_con, int _timeval = 100 );
    ClientSocket*   connect_server_new( const char* _ip, int _port, bool _crypt = true );
    void            multi_client_lock( bool _lock );
#endif
       				
	int				get_timeval() const { return timeval_; }
	void			set_timeout(int timeout) { timeout_ = timeout; } /* 设置超时时间限制*/

	void			refresh_timeout(CommonSocket * csock);
	void			check_timeout();

	int				post_disconnect_msg( DPID dpid, int now_disconn=0, int from=0 );
	int				post_connect_msg(CommonSocket * csock);

    char*           get_ip();

#ifdef DEBUG
public:
	enum { HT_CONNECT = 1, HT_DISCONNECT, HT_BUFFER };
	int ( *hook_func_ )( int, const void* );
	void			set_hook_fn( int(*fn)( int, const void* ) ) { hook_func_ = fn; }
#endif

public:
	BufferQueue*	get_recv_msg(); /*!< 获取接收队列*/
	int				add_recv_msg(Buffer * buf); /*!< 添加接收Buffer*/
	BufferQueue*	get_send_msg(); /*!< 获取发送队列*/
	int				add_send_msg(Buffer * buf); /*!< 添加发送Buffer*/
	int				send_msg(const char * buf, size_t len, DPID dpid = 0);
	void			receive_msg();
	virtual void	msg_handle(const char* msg, int size, int packet_type, DPID dpid) {};

	int				get_max_fd() const { return max_fd_; }
	/*!< 为服务端时,用户数量需减去监听socket，为客户端时user_num_值为1，即连出socket，不过机器人会有多个连出socket*/
	int				get_user_num() const { return (server_mng_ && user_num_ > 0) ? (user_num_ - 1) : user_num_; }
	bool			is_server() const { return server_mng_; }
	bool			is_started() const { return started_; }
	CommonSocket*	get_svr_sock() const { return svr_socket_; }
	CommonSocket*	get_sock(DPID dpid) const;
	CommonSocket*	get_sock_fd(int fd) const;
	const EndPoint*	get_remote_addr() const { return &remote_addr_; }
	int				add_sock(CommonSocket * sock, bool time_out = true);
	int				del_sock(DPID dpid);
	int				do_del_sock();
	inline bool		need_crypt() const { return need_crypt_; }
    inline bool     is_multi_client() const { return multi_client_; }
	SelectThread*	get_select_thread() { return &select_thread_; }
	void            set_pause( bool _flag );
	void            set_more_limit( bool _flag );
    void            set_stop_recv( bool _flag ) { is_stop_recv_ = _flag; }
    bool            is_stop_recv() { return is_stop_recv_; }
    void            set_stop_send( bool _flag ) { is_stop_send_ = _flag; }
    bool            is_stop_send() { return is_stop_send_; }
    void            set_server_id( int _id ) { server_id_ = _id; }
    int             get_server_id() { return server_id_; }
public:
    void            print_packet_stats();
    void            set_packet_stats_open( bool _is_open, ulong _snapshot_type = 0x5007 );
    void            mark_snapshot_stats( packet_type_t _type, int _len  );
private:
    void            reset_packet_stats();
    void            mark_packet_stats( const char* _ptr, bool _is_recv );
public:
    bool            pre_stop_;
    int             proc_thread_num_;
    SockProcThread* sock_proc_thread_;
    Event*          sock_proc_event_;
    Mutex*          get_send_bufq_mutex() { return &send_bufq_mutex_; }
private:
	int				del_usr_list_[MAX_USER];
	int				del_usr_index_;
	int				max_fd_;
	int				user_num_;
	int				timeval_;
	bool			started_;	/*!< NetMng 是否启动*/
	bool			server_mng_; /*!< 是否服务端NetMng*/
	bool			need_crypt_;
	EndPoint		remote_addr_;
	BufferQueue*	recv_buf_queue_;
	BufferQueue*	send_buf_queue_;
	Mutex			recv_bufq_mutex_;
	Mutex			send_bufq_mutex_;
	Mutex			del_usr_mutex_;
	CommonSocket*	svr_socket_;
	CommonSocket*	socket_list_[MAX_USER]; /*!< socket列表,按照fd值为下标存储所有的socket对象指针*/
	SelectThread	select_thread_; /*!< select线程对象*/

	int				timeout_; /* 超时的时间限制*/
	CommonSocket*	timeout_list_head_; /*timeout链表头*/
	CommonSocket*	timeout_list_tail_; /*timeout链表尾*/
    Mutex           timeout_mutex_;

	void			timeout_addtail(CommonSocket * csock);
	void			timeout_unlink(CommonSocket * csock);
    char    ip_[20];
	bool            pause_;
	bool            more_limit_;
    bool            multi_client_;      // 是否多重客户端
    bool            is_stop_recv_;
    bool            is_stop_send_;


    list_head       recv_list_;
    list_head       proc_list_;
    Mutex           recv_lock_;
#ifdef __linux
    Mutex           multi_client_mutex_;
#endif 
    int             server_id_;
private:
    Mutex           buff_stats_mutex_;
    bool            is_buff_stats_open_;
    ulong           snapshot_type_;
    ulong           send_buff_count_[0xFFFF];
    ulong           send_buff_size_[0xFFFF];
    ulong           recv_buff_count_[0xFFFF];
    ulong           recv_buff_size_[0xFFFF];
};

int initial_startup();
void final_cleanup();

extern int g_network_frm;

}

#endif
