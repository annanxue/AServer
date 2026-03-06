#ifndef ASERVER_BF_CLIENT_MODULE_H
#define ASERVER_BF_CLIENT_MODULE_H

#include "define.h"
#include "app_base.h"
#include "net.h"
#include "singleton.h"
#include "thread.h"
#include <string>
#include <vector>

namespace AServer {

struct BFInfo {
    char ip[128];
    bool connected;
    int port;
    int load;
    DPID dpid;
    int heart_beat_time;
};

class ConnectBFThread : public Thread {
public:
    void run() override;
    void init(class BFClient* bf_client, int sleep, int wait, int crypt);

private:
    void send_heart_beat(DPID dpid);
    void send_register(int bf_id, DPID dpid);

    class BFClient* bf_client_ = nullptr;
    int sleep_ = 0;
    int crypt_ = 0;
};

class BFClient : public NetMng {
public:
    BFClient();
    virtual ~BFClient();

    virtual void msg_handle(const char* msg, int size, int packet_type, DPID dpid) override;
    void on_heart_beat_reply(DPID dpid, const char* buf, int buf_size);
    void on_db_save_player(DPID dpid, const char* buf, int buf_size);
    void on_connect(DPID dpid, const char* buf, int buf_size);
    void on_disconnect(DPID dpid, const char* buf, int buf_size);
    void on_save_player(DPID dpid, const char* buf, int buf_size);

    void start_connect(int sleep_time, int wait_time, int crypt);
    void stop_connect();
    BFInfo* get_bf_info() { return bf_info_; }
    int get_bf_num() { return bf_num_; }
    void set_bf_num(int num);

    int choose_bfid_min_load();
    int choose_bfid_random();
    int choose_bfid_by_serverid();
    DPID choose_dpid_min_load();
    bool is_bf_ready();

private:
    BFInfo* bf_info_ = nullptr;
    int bf_num_ = 0;
    ConnectBFThread connect_thread_;
    bool running_ = false;
};

class BFClientModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

#define g_bfclient AServer::BFClient::instance_ptr()

}

#endif
