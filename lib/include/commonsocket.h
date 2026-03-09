#ifndef __COMMON_SOCKET_H__
#define __COMMON_SOCKET_H__

#include "ffsys.h"
#include "autolock.h"
#include "endpoint.h"
#include "socket.h"
#include "buffer.h"
#include "log.h"


#ifdef __linux
	#include "pike.h"
	#include "flow_rate.h"
#else
	#include "pike_lua.h"
#endif

namespace FF_Network {

class NetMng;

class CommonSocket : public Socket
{
protected:
	CommonSocket():Socket(-1) {} /*!< 防止调用默认构造函数 */
public:
	CommonSocket(int sock, bool crypt);
	CommonSocket(bool crypt);
	virtual ~CommonSocket();

	void			init();
	DPID			get_dpid() const;
	const EndPoint*	get_endpoint() const;

    in_addr_t       get_ip() { return peer_addr_.GetIP(); } 
    in_addr_t       get_real_ip() { return real_ip_; }
    void            set_real_ip( in_addr_t real_ip ) { real_ip_ = real_ip; }

public:
	int				tag_;

	virtual int		process(int events, NetMng * parent) = 0; /*!< 继承这个函数来处理io事件*/
	int				do_send();
	int				do_recv(Buffer ** buf);
	int				do_connect(const EndPoint & ep);
	int				do_disconnect();
	int				do_listen(EndPoint & ep, int backlog);

	int				send_msg(Buffer * buffer);
	void			set_peer(EndPoint peer);

	void			set_seed(int seed);
	bool			need_crypt() const { return need_crypt_; }
	int				send_seed();
	int				recv_seed();

    bool            is_ok(){ return is_ok_; }
    void            set_bad();


	CommonSocket*	timeout_prev_; /* 用于链接超时队列*/
	CommonSocket*	timeout_next_;
	int				timeout_; /* 超时时间*/

    BufferQueue*    get_send_bufque() { return &send_bufque_; }
    Mutex*          get_send_mutex() { return &send_mutex_; }
private:
	int				seed_;
	bool			need_crypt_;
	Buffer*			recv_buf_;
	BufferQueue		send_bufque_;
    Mutex           send_mutex_;

    volatile bool   is_ok_;

#ifdef __linux
	ff_ctx			ctx_send_, ctx_recv_;
#endif
	

	int				act_send(const char * buf, int length); /*!< 实际发送函数*/
	int				act_recv(char * buf, int length);/*!< 实际接收函数*/
private:
	static int serial_no_;
	DPID			dpid_;	/*!< 使用一个自增的serial number以及该socket的fd值来生成标识该次连接的dpid.
								dpid的高16位为serial number,低16位为该socket的fd值. */

	EndPoint		peer_addr_;
#ifdef __linux
	FlowRate		flow_rate_recv_;
	FlowRate		flow_rate_send_;
#endif

    in_addr_t       real_ip_;

public:
    static int send_buf_num;
    static int recv_buf_num;

    static Mutex send_lock;
    static void send_up()   { AutoLock lock(&send_lock); send_buf_num++; }
    static void send_down() { AutoLock lock(&send_lock); send_buf_num--; }

    static void recv_up()   { AutoLock lock(&recv_lock); recv_buf_num++; }
    static void recv_down() { AutoLock lock(&recv_lock); recv_buf_num--; }
    static Mutex recv_lock;

    static int  get_send_num() { return send_buf_num; }
    static int  get_recv_num() { return recv_buf_num; }
};

}

#endif
