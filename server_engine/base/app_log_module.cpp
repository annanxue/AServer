#include "app_log_module.h"
#include "log.h"

bool LogModule::app_class_init(){
    char    log_folder_[MAX_PATH] = {0}; 
    int use_syslog = 0;
    char proc_flag[FILE_FLAG_LEN] = {0};
    char syslog_list[MAX_LOG_TYPE] = {0};
    int elv = 4, llv = 2, tlv = 2, plv = 2;
	int use_old_log_version = 0;

    APP_GET_TABLE("Log",2);   
	APP_GET_NUMBER("UseLogOldVersion", use_old_log_version);
    APP_GET_STRING("LogPathPrefix", log_folder_ );
    APP_GET_NUMBER("ErrLevel",  elv);
    APP_GET_NUMBER("LogLevel",  llv);
    APP_GET_NUMBER("TraceLevel",  tlv);
    APP_GET_NUMBER("ProfLevel",  plv);
    APP_END_TABLE();

    int is_syslog_open = 0;
    APP_GET_NUMBER("IsSyslogOpen",  is_syslog_open);

    if( g_log->startup( 
            log_folder_, 
            app_base_->is_daemon(), 
            use_syslog,
            proc_flag, 
            syslog_list , 
			use_old_log_version ) )
    {
        FLOG( "./error.log", "failed to init log" );
        return false;
    }
    g_log->set_level( 0, elv );
    g_log->set_level( 1, llv );
    g_log->set_level( 2, tlv );
    g_log->set_level( LOG_TYPE_PROF, plv );
    g_log->set_is_syslog_open(is_syslog_open==1);

    return true;
}
bool LogModule::app_class_destroy(){
    LOG(2)( "===========Log Module Destroy===========");
    return true;
}

