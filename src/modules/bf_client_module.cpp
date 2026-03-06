#include "bf_client_module.h"
#include "game_lua_module.h"
#include "log.h"
#include <cstring>

namespace AServer {

void ConnectBFThread::init(BFClient* bf_client, int sleep, int wait, int crypt) {
    bf_client_ = bf_client;
    sleep_ = sleep;
    crypt_ = crypt;
}

void ConnectBFThread::run() {
    INFO("ConnectBFThread started");
    while (is_running()) {
        if (bf_client_) {
            bf_client_->start_connect(sleep_, 0, crypt_);
        }
        std::this_thread::sleep_for(std::chrono::seconds(sleep_));
    }
    INFO("ConnectBFThread stopped");
}

BFClient::BFClient() {
}

BFClient::~BFClient() {
    stop_connect();
    if (bf_info_) {
        delete[] bf_info_;
        bf_info_ = nullptr;
    }
}

void BFClient::set_bf_num(int num) {
    bf_num_ = num;
    if (bf_info_) {
        delete[] bf_info_;
    }
    bf_info_ = new BFInfo[bf_num_];
    memset(bf_info_, 0, sizeof(BFInfo) * bf_num_);
}

void BFClient::msg_handle(const char* msg, int size, int packet_type, DPID dpid) {
    switch (packet_type) {
        case PT_CONNECT:
            on_connect(dpid, msg, size);
            break;
        case PT_DISCONNECT:
            on_disconnect(dpid, msg, size);
            break;
        default:
            if (g_luasvr) {
                lua_State* L = g_luasvr->L();
                lua_getglobal(L, "bfclient");
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "on_message");
                    if (lua_isfunction(L, -1)) {
                        lua_pushlstring(L, msg, size);
                        lua_pushinteger(L, size);
                        lua_pushinteger(L, packet_type);
                        lua_pushinteger(L, dpid);
                        LuaServer::lua_pcall_ex(L, 4, 0, 0);
                    }
                }
                lua_settop(L, 0);
            }
            break;
    }
}

void BFClient::on_heart_beat_reply(DPID dpid, const char* buf, int buf_size) {
    INFO("Heart beat reply from BF: %u", dpid);
}

void BFClient::on_db_save_player(DPID dpid, const char* buf, int buf_size) {
    INFO("DB save player from BF: %u", dpid);
}

void BFClient::on_connect(DPID dpid, const char* buf, int buf_size) {
    INFO("BFClient connected, dpid: %u", dpid);
}

void BFClient::on_disconnect(DPID dpid, const char* buf, int buf_size) {
    INFO("BFClient disconnected, dpid: %u", dpid);
}

void BFClient::on_save_player(DPID dpid, const char* buf, int buf_size) {
    INFO("Save player from BF: %u", dpid);
}

void BFClient::start_connect(int sleep_time, int wait_time, int crypt) {
    INFO("BFClient start connect...");
}

void BFClient::stop_connect() {
    connect_thread_.stop();
    connect_thread_.join();
}

int BFClient::choose_bfid_min_load() {
    int min_load = INT_MAX;
    int min_id = -1;
    for (int i = 0; i < bf_num_; ++i) {
        if (bf_info_[i].connected && bf_info_[i].load < min_load) {
            min_load = bf_info_[i].load;
            min_id = i;
        }
    }
    return min_id;
}

int BFClient::choose_bfid_random() {
    std::vector<int> valid_ids;
    for (int i = 0; i < bf_num_; ++i) {
        if (bf_info_[i].connected) {
            valid_ids.push_back(i);
        }
    }
    if (valid_ids.empty()) return -1;
    return valid_ids[rand() % valid_ids.size()];
}

int BFClient::choose_bfid_by_serverid() {
    return 0;
}

DPID BFClient::choose_dpid_min_load() {
    int min_load = INT_MAX;
    DPID min_dpid = 0;
    for (int i = 0; i < bf_num_; ++i) {
        if (bf_info_[i].connected && bf_info_[i].load < min_load) {
            min_load = bf_info_[i].load;
            min_dpid = bf_info_[i].dpid;
        }
    }
    return min_dpid;
}

bool BFClient::is_bf_ready() {
    for (int i = 0; i < bf_num_; ++i) {
        if (bf_info_[i].connected) {
            return true;
        }
    }
    return false;
}

bool BFClientModule::app_class_init() {
    INFO("BFClientModule initializing...");
    return true;
}

bool BFClientModule::app_class_destroy() {
    INFO("BFClientModule destroying...");
    return true;
}

}
