#ifndef ASERVER_LOGIN_CLIENT_MODULE_H
#define ASERVER_LOGIN_CLIENT_MODULE_H

#include "define.h"
#include "app_base.h"
#include "net.h"
#include "singleton.h"
#include <string>

namespace AServer {

class LoginClient : public NetMng {
public:
    LoginClient();
    virtual ~LoginClient();

    virtual void msg_handle(const char* msg, int size, int packet_type, DPID dpid) override;

private:
    void init_func_map();
    void on_connect(DPID dpid, const char* buf, int buf_size);
    void on_disconnect(DPID dpid, const char* buf, int buf_size);
    void send_exec_sql(const char* sql);
    void lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid);
    void send_register_server_id();
};

class LoginClientModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

#define g_loginclient AServer::LoginClient::instance_ptr()

}

#endif
