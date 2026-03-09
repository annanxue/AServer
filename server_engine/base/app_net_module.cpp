#include "app_net_module.h"
#include "netmng.h"
#include "log.h"

bool NetModule::app_class_init()
{ 
    FF_Network::initial_startup(); 
    LOG(2)( "===========Network Module Start===========");

    return true; 
}

bool NetModule::app_class_destroy()
{
    FF_Network::final_cleanup();
    LOG(2)( "===========Network Module Destroy===========");

    return true;         
}
