#include "net_module.h"
#include "game_svr_module.h"
#include "db_client_module.h"
#include "bf_client_module.h"
#include "login_client_module.h"
#include "log_client_module.h"
#include "gm_client_module.h"
#include "gm_svr_module.h"
#include "log.h"

namespace AServer {

bool NetModule::app_class_init() {
    INFO("NetModule initializing...");

    g_gamesvr->init();
    g_dbclient->init();
    g_bfclient->init();
    g_loginclient->init();
    g_logclient->init();
    g_gmclient->init();
    g_gmsvr->init();

    INFO("NetModule initialized");
    return true;
}

bool NetModule::app_class_destroy() {
    INFO("NetModule destroying...");

    g_gamesvr->destroy();
    g_dbclient->destroy();
    g_bfclient->destroy();
    g_loginclient->destroy();
    g_logclient->destroy();
    g_gmclient->destroy();
    g_gmsvr->destroy();

    return true;
}

}
