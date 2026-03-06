#include "net.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace AServer {

uint32_t CommonSocket::get_tick() {
#ifdef _WIN32
    return GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

Buffer::Buffer(const char* data, int size, DPID dpid, int packet_type)
    : size(size), dpid(dpid), packet_type(packet_type) {
    capacity = size + 1024;
    this->data = new char[capacity];
    memcpy(this->data, data, size);
}

Buffer::~Buffer() {
    if (data) {
        delete[] data;
        data = nullptr;
    }
}

void Buffer::resize(int new_size) {
    if (new_size > capacity) {
        ensure_capacity(new_size);
    }
    size = new_size;
}

void Buffer::ensure_capacity(int cap) {
    if (cap > capacity) {
        capacity = cap * 2;
        char* new_data = new char[capacity];
        if (data && size > 0) {
            memcpy(new_data, data, size);
        }
        delete[] data;
        data = new_data;
    }
}

BufferQueue::BufferQueue() {
}

BufferQueue::~BufferQueue() {
    clear();
}

void BufferQueue::push(Buffer* buf) {
    std::lock_guard<Mutex> lock(mutex_);
    queue_.push(buf);
}

Buffer* BufferQueue::pop() {
    std::lock_guard<Mutex> lock(mutex_);
    if (queue_.empty()) {
        return nullptr;
    }
    Buffer* buf = queue_.front();
    queue_.pop();
    return buf;
}

bool BufferQueue::empty() const {
    std::lock_guard<Mutex> lock(mutex_);
    return queue_.empty();
}

int BufferQueue::size() const {
    std::lock_guard<Mutex> lock(mutex_);
    return (int)queue_.size();
}

void BufferQueue::clear() {
    std::lock_guard<Mutex> lock(mutex_);
    while (!queue_.empty()) {
        Buffer* buf = queue_.front();
        queue_.pop();
        delete buf;
    }
}

CommonSocket::CommonSocket()
    : ip_{0} {
}

CommonSocket::~CommonSocket() {
    if (sock_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(sock_);
#else
        close(sock_);
#endif
        sock_ = INVALID_SOCKET;
    }
}

int CommonSocket::send_data(const char* data, int len) {
    if (sock_ == INVALID_SOCKET || !connected_) {
        return -1;
    }

    int total_sent = 0;
    while (total_sent < len) {
        int sent = send(sock_, data + total_sent, len - total_sent, 0);
        if (sent < 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                continue;
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
#endif
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

int CommonSocket::recv_data(char* buf, int len) {
    if (sock_ == INVALID_SOCKET) {
        return -1;
    }
    return recv(sock_, buf, len, 0);
}

NetMng::NetMng() {
    memset(func_map_, 0, sizeof(func_map_));
}

NetMng::~NetMng() {
    destroy();
}

int NetMng::init() {
#ifdef _WIN32
    static bool wsainit = false;
    if (!wsainit) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        wsainit = true;
    }
#endif
    return 0;
}

int NetMng::destroy() {
    if (server_socket_) {
        delete server_socket_;
        server_socket_ = nullptr;
    }

    std::lock_guard<Mutex> lock(sock_mutex_);
    for (auto& pair : sock_map_) {
        delete pair.second;
    }
    sock_map_.clear();

    recv_queue_.clear();
    send_queue_.clear();

    return 0;
}

int NetMng::start_server(int max_con, int port, bool crypt, const char* ip, int timeval, int pthread_num) {
    if (started_) {
        return 0;
    }

    timeval_ = timeval;
    user_num_ = max_con;

    if (init_connection(port, ip ? ip : "0.0.0.0") != 0) {
        return -1;
    }

    if (create_session() != 0) {
        return -1;
    }

    started_ = true;
    server_mng_ = true;
    INFO("Server started on port %d, max connections: %d", port, max_con);
    return 0;
}

int NetMng::init_connection(int port, const char* addr) {
    server_socket_ = new CommonSocket();
    server_socket_->set_server(true);

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        ERR("Failed to create socket");
        return -1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons((uint16_t)port);

    if (!addr || strcmp(addr, "0.0.0.0") == 0) {
        addr_in.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, addr, &addr_in.sin_addr);
    }

    if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)) < 0) {
        ERR("Failed to bind socket on port %d", port);
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    server_socket_->set_sock(sock);
    return 0;
}

int NetMng::create_session() {
    if (!server_socket_) {
        return -1;
    }

    if (listen(server_socket_->sock(), 5) < 0) {
        ERR("Failed to listen");
        return -1;
    }

    return 0;
}

int NetMng::connect_server(const char* addr, int port, bool crypt, int timeval) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return -1;
    }

    struct hostent* he = gethostbyname(addr);
    if (!he) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons((uint16_t)port);
    memcpy(&addr_in.sin_addr, he->h_addr_list[0], he->h_length);

    if (::connect(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    CommonSocket* cs = new CommonSocket();
    cs->set_sock(sock);
    cs->set_connected(true);
    cs->set_dpid(1);
    cs->set_ip(inet_ntoa(addr_in.sin_addr));

    std::lock_guard<Mutex> lock(sock_mutex_);
    sock_map_[cs->get_dpid()] = cs;
    user_num_ = 1;

    INFO("Connected to server %s:%d", addr, port);
    return 0;
}

int NetMng::stop_server() {
    started_ = false;
    if (server_socket_) {
        server_socket_->set_connected(false);
    }
    return 0;
}

int NetMng::join_session() {
    return 0;
}

int NetMng::init_multi_client(int max_con, int timeval) {
    user_num_ = max_con;
    timeval_ = timeval;
    return 0;
}

CommonSocket* NetMng::connect_server_new(const char* ip, int port, bool crypt) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return nullptr;
    }

    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, ip, &addr_in.sin_addr);

    if (::connect(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return nullptr;
    }

    CommonSocket* cs = new CommonSocket();
    cs->set_sock(sock);
    cs->set_connected(true);
    cs->set_ip(ip);

    std::lock_guard<Mutex> lock(sock_mutex_);
    DPID dpid = (DPID)sock;
    cs->set_dpid(dpid);
    sock_map_[dpid] = cs;

    post_connect_msg(cs);
    INFO("Connected to %s:%d, dpid: %u", ip, port, dpid);
    return cs;
}

void NetMng::refresh_timeout(CommonSocket* csock) {
    if (csock) {
        csock->update_heartbeat();
    }
}

void NetMng::check_timeout() {
    if (timeout_ <= 0) return;

    uint32_t now = CommonSocket::get_tick();
    std::lock_guard<Mutex> lock(sock_mutex_);

    std::vector<DPID> to_remove;
    for (auto& pair : sock_map_) {
        CommonSocket* cs = pair.second;
        if (cs->is_server()) continue;

        uint32_t diff = now - cs->get_heartbeat();
        if (diff > (uint32_t)timeout_) {
            to_remove.push_back(pair.first);
        }
    }

    for (DPID dpid : to_remove) {
        del_sock(dpid);
        post_disconnect_msg(dpid);
    }
}

int NetMng::post_disconnect_msg(DPID dpid, int now_disconn, int from) {
    return 0;
}

int NetMng::post_connect_msg(CommonSocket* csock) {
    return 0;
}

int NetMng::add_recv_msg(Buffer* buf) {
    recv_queue_.push(buf);
    return 0;
}

int NetMng::add_send_msg(Buffer* buf) {
    send_queue_.push(buf);
    return 0;
}

int NetMng::send_msg(const char* buf, size_t len, DPID dpid) {
    std::lock_guard<Mutex> lock(sock_mutex_);

    if (dpid == 0) {
        for (auto& pair : sock_map_) {
            CommonSocket* cs = pair.second;
            cs->send_data(buf, (int)len);
        }
    } else {
        auto it = sock_map_.find(dpid);
        if (it != sock_map_.end()) {
            return it->second->send_data(buf, (int)len);
        }
    }
    return 0;
}

void NetMng::receive_msg() {
    if (!started_) return;

    if (server_mng_ && server_socket_) {
        struct sockaddr_in addr_in;
        socklen_t addr_len = sizeof(addr_in);
        socket_t client_sock = accept(server_socket_->sock(), (struct sockaddr*)&addr_in, &addr_len);

        if (client_sock != INVALID_SOCKET) {
            CommonSocket* cs = new CommonSocket();
            cs->set_sock(client_sock);
            cs->set_connected(true);
            cs->set_ip(inet_ntoa(addr_in.sin_addr));

            std::lock_guard<Mutex> lock(sock_mutex_);
            DPID dpid = (DPID)client_sock;
            cs->set_dpid(dpid);
            sock_map_[dpid] = cs;

            post_connect_msg(cs);
            INFO("New connection from %s, dpid: %u", cs->get_ip(), dpid);
        }
    }

    std::lock_guard<Mutex> lock(sock_mutex_);
    std::vector<DPID> to_remove;

    for (auto& pair : sock_map_) {
        CommonSocket* cs = pair.second;
        if (cs->is_server()) continue;

        char buf[4096];
        int len = cs->recv_data(buf, sizeof(buf));
        if (len > 0) {
            Buffer* buffer = new Buffer(buf, len, cs->get_dpid(), PT_DATA);
            add_recv_msg(buffer);
        } else if (len < 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                to_remove.push_back(pair.first);
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                to_remove.push_back(pair.first);
            }
#endif
        }
    }

    for (DPID dpid : to_remove) {
        del_sock(dpid);
        post_disconnect_msg(dpid);
    }
}

void NetMng::del_sock(DPID dpid) {
    std::lock_guard<Mutex> lock(sock_mutex_);
    auto it = sock_map_.find(dpid);
    if (it != sock_map_.end()) {
        CommonSocket* cs = it->second;
        cs->set_connected(false);
        delete cs;
        sock_map_.erase(it);
        INFO("Socket disconnected, dpid: %u", dpid);
    }
}

CommonSocket* NetMng::get_sock(DPID dpid) {
    std::lock_guard<Mutex> lock(sock_mutex_);
    auto it = sock_map_.find(dpid);
    if (it != sock_map_.end()) {
        return it->second;
    }
    return nullptr;
}

}
