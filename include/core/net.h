#ifndef ASERVER_NET_H
#define ASERVER_NET_H

#include "define.h"
#include "thread.h"
#include "singleton.h"
#include "log.h"
#include <map>
#include <queue>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#endif

namespace AServer {

using DPID = uint32_t;

enum PacketType {
    PT_CONNECT = 1,
    PT_DISCONNECT = 2,
    PT_DATA = 3,
    PT_PACKET_MAX = 256
};

struct Buffer {
    char* data = nullptr;
    int size = 0;
    int capacity = 0;
    DPID dpid = 0;
    int packet_type = 0;

    Buffer() = default;
    Buffer(const char* data, int size, DPID dpid = 0, int packet_type = PT_DATA);
    ~Buffer();

    void resize(int new_size);
    void ensure_capacity(int capacity);
};

class CommonSocket;

class BufferQueue {
public:
    BufferQueue();
    ~BufferQueue();

    void push(Buffer* buf);
    Buffer* pop();
    bool empty() const;
    int size() const;
    void clear();

private:
    std::queue<Buffer*> queue_;
    mutable Mutex mutex_;
};

class NetMng : public Singleton<NetMng> {
public:
    NetMng();
    virtual ~NetMng();

    virtual int init();
    virtual int destroy();

    virtual int start_server(int max_con, int port, bool crypt = true, 
        const char* ip = nullptr, int timeval = 100, int pthread_num = 1);
    int create_session();
    int connect_server(const char* addr, int port, bool crypt = true, int timeval = 100);
    int stop_server();

    int init_connection(int port, const char* addr = nullptr);
    int join_session();

    int init_multi_client(int max_con, int timeval = 100);
    CommonSocket* connect_server_new(const char* ip, int port, bool crypt = true);

    int get_timeval() const { return timeval_; }
    void set_timeout(int timeout) { timeout_ = timeout; }

    void refresh_timeout(CommonSocket* csock);
    void check_timeout();

    int post_disconnect_msg(DPID dpid, int now_disconn = 0, int from = 0);
    int post_connect_msg(CommonSocket* csock);

    virtual void msg_handle(const char* msg, int size, int packet_type, DPID dpid) {}

    BufferQueue* get_recv_msg() { return &recv_queue_; }
    int add_recv_msg(Buffer* buf);
    BufferQueue* get_send_msg() { return &send_queue_; }
    int add_send_msg(Buffer* buf);
    int send_msg(const char* buf, size_t len, DPID dpid = 0);
    void receive_msg();

    int get_max_fd() const { return max_fd_; }
    int get_user_num() const { return user_num_; }
    bool is_server() const { return server_mng_; }
    bool is_started() const { return started_; }
    CommonSocket* get_svr_sock() const { return server_socket_; }

    void del_sock(DPID dpid);
    CommonSocket* get_sock(DPID dpid);

    void set_pause(bool pause) { pause_ = pause; }

protected:
    virtual void on_connect(DPID dpid) {}
    virtual void on_disconnect(DPID dpid) {}

    bool server_mng_ = false;
    bool started_ = false;
    bool pause_ = false;
    int max_fd_ = 0;
    int user_num_ = 0;
    int timeval_ = 100;
    int timeout_ = 30000;

    CommonSocket* server_socket_ = nullptr;
    std::map<DPID, CommonSocket*> sock_map_;

    BufferQueue recv_queue_;
    BufferQueue send_queue_;

    mutable Mutex sock_mutex_;

    typedef void (NetMng::*PacketHandler)(const char*, int, int, DPID);
    PacketHandler func_map_[PT_PACKET_MAX];
};

class CommonSocket {
public:
    CommonSocket();
    virtual ~CommonSocket();

    socket_t sock() { return sock_; }
    void set_sock(socket_t s) { sock_ = s; }

    DPID get_dpid() const { return dpid_; }
    void set_dpid(DPID dpid) { dpid_ = dpid; }

    const char* get_ip() const { return ip_; }
    void set_ip(const char* ip) { strncpy(ip_, ip, sizeof(ip_) - 1); }

    bool is_connected() const { return connected_; }
    void set_connected(bool connected) { connected_ = connected; }

    bool is_server() const { return is_server_; }
    void set_server(bool is_server) { is_server_ = is_server; }

    int send_data(const char* data, int len);

    void update_heartbeat() { last_heartbeat_ = get_tick(); }
    uint32_t get_heartbeat() const { return last_heartbeat_; }

    static uint32_t get_tick();

    virtual int recv_data(char* buf, int len);
    virtual void on_connect() {}
    virtual void on_disconnect() {}

protected:
    socket_t sock_ = INVALID_SOCKET;
    DPID dpid_ = 0;
    char ip_[32];
    bool connected_ = false;
    bool is_server_ = false;
    uint32_t last_heartbeat_ = 0;
};

}

#endif
