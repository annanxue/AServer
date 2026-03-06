#ifndef ASERVER_GM_CLIENT_MODULE_H
#define ASERVER_GM_CLIENT_MODULE_H

#include "define.h"
#include "app_base.h"
#include "net.h"
#include "singleton.h"
#include <string>

namespace AServer {

class GMClient : public NetMng {
public:
    GMClient();
    virtual ~GMClient();

    virtual void msg_handle(const char* msg, int size, int packet_type, DPID dpid) override;

private:
    void init_func_map();
    void on_connect(DPID dpid, const char* buf, int buf_size);
    void on_disconnect(DPID dpid, const char* buf, int buf_size);
    void lua_msg_handle(lua_State* L, const char* msg, int size, int packet_type, DPID dpid);
};

class GMClientModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

#define g_gmclient AServer::GMClient::instance_ptr()

}

#endif
