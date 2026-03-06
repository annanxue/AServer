#ifndef ASERVER_GAME_APP_H
#define ASERVER_GAME_APP_H

#include "define.h"
#include "app_base.h"
#include "logic_thread.h"
#include "game_svr_module.h"
#include "game_lua_module.h"
#include "log_module.h"
#include "net_module.h"
#include "timer_module.h"
#include "db_client_module.h"
#include "bf_client_module.h"
#include "login_client_module.h"
#include "log_client_module.h"
#include "gm_client_module.h"
#include "gm_svr_module.h"
#include <csignal>
#include <iostream>

namespace AServer {

class GameThread : public LogicThread {
public:
    GameThread() {
        set_thread_name("GameThread");
        set_frame_time(100);
    }

    virtual void lock() override {
        if (g_luasvr) {
            g_luasvr->lock();
        }
    }

    virtual void unlock() override {
        if (g_luasvr) {
            g_luasvr->unlock();
        }
    }

    virtual void reload_lua() override {
        if (reload_lua_ && g_luasvr) {
            g_luasvr->reload();
            reload_lua_ = false;
            INFO("Lua scripts reloaded");
        }
    }

    virtual void check_stop() override {
        if (pre_stop_ && !pre_stop_tried_) {
            pre_stop_tried_ = true;
            if (g_gamesvr) {
                g_gamesvr->set_pause(true);
                g_gamesvr->kick_all_player();
                g_gamesvr->on_server_pre_stop();
            }
            INFO("Server pre-stop initiated");
            AppBase::active_ = false;
        }
    }

    virtual void recv_msg() override {
        if (g_gamesvr) g_gamesvr->receive_msg();
        if (g_dbclient) g_dbclient->receive_msg();
        if (g_bfclient) g_bfclient->receive_msg();
        if (g_loginclient) g_loginclient->receive_msg();
        if (g_logclient) g_logclient->receive_msg();
        if (g_gmclient) g_gmclient->receive_msg();
        if (g_gmsvr) g_gmsvr->receive_msg();
    }

    virtual void process() override {
    }

    virtual void do_logic_state_lua() override {
    }

    virtual void send_msg() override {
    }

    virtual void run_timer() override {
        if (g_timermng) {
            g_timermng->run_timer_list();
        }
    }

    virtual void run_gc_step(int32_t frame_left_time) override {
        if (g_luasvr) {
            g_luasvr->check_gc(frame_left_time);
        }
    }

    virtual void log_frame() override {
    }
};

class GameApp : public AppBase {
public:
    GameApp() = default;
    virtual ~GameApp() = default;

protected:
    virtual bool main_loop() override {
        INFO("Entering main loop...");

        GameThread game_thread;
        game_thread.start();

        while (AppBase::active_) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }

        game_thread.join();
        INFO("Main loop exited");
        return true;
    }

private:
    virtual void on_signal(int sig) override {
        INFO("Received signal: %d", sig);
        if (sig == SIGINT || sig == SIGTERM) {
            AppBase::active_ = false;
        }
    }
};

}

static void signal_handler(int sig) {
    INFO("Signal handler: %d", sig);
    AServer::AppBase::active_ = false;
}

int main(int argc, char* argv[]) {
    INFO("========================================");
    INFO("  AServer Game Server Starting...");
    INFO("========================================");

    AServer::GameApp app;

    if (!app.init(argc, argv)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }

    app.register_signal(SIGINT, signal_handler);
#ifndef _WIN32
    app.register_signal(SIGPIPE, SIG_IGN);
#endif

    AServer::LogModule log_module;
    AServer::NetModule net_module;
    AServer::TimerModule timer_module;
    AServer::LuaModule lua_module;
    AServer::GameSvrModule gsvr_module;
    AServer::DBClientModule dbclient_module;
    AServer::BFClientModule bfclient_module;
    AServer::LoginClientModule loginclient_module;
    AServer::GMClientModule gmclient_module;
    AServer::LogClientModule logclient_module;
    AServer::GmSvrModule gmsvr_module;

    app.register_class(&log_module);
    app.register_class(&timer_module);
    app.register_class(&net_module);
    app.register_class(&lua_module);
    app.register_class(&gsvr_module);
    app.register_class(&dbclient_module);
    app.register_class(&bfclient_module);
    app.register_class(&loginclient_module);
    app.register_class(&gmclient_module);
    app.register_class(&logclient_module);
    app.register_class(&gmsvr_module);

    if (!app.start()) {
        std::cerr << "Failed to start application" << std::endl;
        return 1;
    }

    INFO("========================================");
    INFO("  AServer Game Server Stopped");
    INFO("========================================");
    return 0;
}

#endif
