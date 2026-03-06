#ifndef ASERVER_NET_MODULE_H
#define ASERVER_NET_MODULE_H

#include "define.h"
#include "app_base.h"
#include "singleton.h"

namespace AServer {

class NetModule : public AppClassInterface {
public:
    bool app_class_init() override;
    bool app_class_destroy() override;
};

}

#endif
