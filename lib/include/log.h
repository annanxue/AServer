#ifndef __NLOG_H__
#define __NLOG_H__

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <syslog.h>
#include "event.h"
#include "thread.h"
#include "define.h"
#include "misc.h"
#include "autolock.h"
#include "singleton.h"

#define LOG_TYPE_ERROR         0
#define LOG_TYPE_LOG           1
#define LOG_TYPE_TRACE         2
#define LOG_TYPE_ITEM_CREATE   3
#define LOG_TYPE_ITEM_TRADE    4
#define LOG_TYPE_ITEM_USE      5
#define LOG_TYPE_GM            6
#define LOG_TYPE_PLAYER        7 
#define LOG_TYPE_EVENT         8
#define LOG_TYPE_LOGIN         9
#define LOG_TYPE_SAFE          10
#define LOG_TYPE_POPO          11
#define LOG_TYPE_SAVE          12
#define LOG_TYPE_PROF          13
#define LOG_TYPE_TEST          14
#define LOG_TYPE_SELLS         15
#define LOG_TYPE_TRIGGER       16
#define LOG_TYPE_INSTANCE      17
#define LOG_TYPE_MONEY         18
#define LOG_TYPE_ITEM_BIND     19
#define LOG_TYPE_SYSLOG        20
#define LOG_TYPE_FIGHT         21
#define LOG_TYPE_SDKLOG        22
#define LOG_TYPE_BACKUP        23
#define LOG_TYPE_ONLINE        24
#define LOG_TYPE_TGALOG        25

#define MAX_LOG_TYPE           26       //日志的种类数量

static const char* TERM_NO_COLOR = "";
static const char* LOG_TYPE_STR[] = {
    "ERROR ",
    "LOG ",
    "TRACE ",
    "ITEM_CREATE ",
    "ITEM_TRADE ",
    "ITEM_USE ",
    "GM ",
    "PLAYER ",
    "EVENT ",
    "LOGIN ",
    "SAFE ",
    "POPO ",
    "SAVE ",
    "PROF ",
    "TEST ",
    "SELLS ",
    "TRIGGER ",
    "INSTANCE ",
    "MONEY ",
    "ITEM_BIND ",
    "SYSLOG ",
    "FIGHT ",
};
#define MAX_LOG_LEVEL          5		//日志的分级
#define MAX_LINE_NUM           3000000	//循环日志的行数
#define MAX_FILE_NUM           20		//循环日志的文件数

#define FILE_FLAG_LEN          16


class LogBuffer
{
public:
    list_head link_;
	int type;
	int len;
    const char* color;
	char buff[MAX_BUFF_LEN];

	LogBuffer()
    {
        INIT_LIST_HEAD( &link_ );
        color = TERM_NO_COLOR;
        buff[0] = '\0';
    };

	virtual ~LogBuffer(){ };
};

class NLog: public Thread
{
public:
	NLog();
	~NLog();
	typedef void (NLog::* logapi_t)( const char* format, ... );

	int reset_log_file_handle(); 
	void migrate_log_file(); 
	int			    startup( const char* _log_path, int _daemon_mode = 0, int _use_syslog=0, const char* _proc_flag="", char* _syslog_list=NULL  , bool _use_old_log_version = false );
	void			shutdown();
	void			set_level(int _type, int _level);              //设置指定类型的日志等级，大于等级的不记录
	void			write_null( const char* format, ... );
	//void 			do_write( int _type );
	logapi_t 		get_logf( unsigned int _i, unsigned int _n );	

	void			process_timer();

	static void		write_file( const char* _filename, const char* format, ... );

	void			run();
    char*           get_time_str(){ return time_str_; }
    Mutex*          get_write_mutex() { return &mtx_; }
private:
    bool execute_shell_cmd(const char* cmd);


	int				time_len_;
	char			time_str_[256];

    bool            is_syslog_open_;
    int				syslog_len_;
	char			syslog_str_[256];
    int             date_year_;
    int             date_mon_;
    int             date_mday_;
    int             date_hour_;

	int				olog_fn_;				//循环日志的文件号
	int				olog_ln_;				//循环日志的行号
	int				otrace_fn_;				//循环日志的文件号
	int				otrace_ln_;				//循环日志的行号


    Mutex           mtx_;

	int				daemon_mode_;
	char			log_path_[MAX_PATH];	//循环log，记录一定时期的log，备案
	char			init_log_path_[MAX_PATH];	
    char            temp_time_str_[MAX_PATH];

	FILE*			olog_;
	FILE*			otrace_;
	FILE*			stdi_;
	FILE*			stdo_;
	FILE*			stde_;

	FILE*			log_[MAX_LOG_TYPE];   //日志文件句柄
	
	void			check_olog();
	void			check_otrace();
	FILE*			open_log_file( const char *name );

	Event			write_event_;
	
	//std::list<LogBuffer*> write_list_;
	list_head       write_list_;

    char            proc_flag_[FILE_FLAG_LEN];

	bool			use_old_log_version_; 


private:
    void change_color( int _type, LogBuffer* _buffer );
    void write_to_stdout_on_non_daemon( LogBuffer* _buffer );
    void process_syslog_open( int year, int mon, int mday, int hour);
public:
    inline void set_is_syslog_open( bool val )
    {
        is_syslog_open_ = val;
    }

    template<int type>
	void write_log( const char* format, ... )
	{
        if( !active_ )
        {
            return;
        }

		LogBuffer *buffer = new LogBuffer; 

        if ( NULL == buffer )
        {
            return;
        }
        
        if( type != LOG_TYPE_SYSLOG && type != LOG_TYPE_SDKLOG && type != LOG_TYPE_BACKUP && type != LOG_TYPE_ONLINE && type != LOG_TYPE_TGALOG )
        {
            change_color( type, buffer );

            strncat( buffer->buff, time_str_, MAX_BUFF_LEN );
            if (!daemon_mode_) {
                assert(type < sizeof(LOG_TYPE_STR)/sizeof(char*));
                strncat(buffer->buff, LOG_TYPE_STR[type], MAX_BUFF_LEN);
            }
        }

        int buffer_write_index = strlen( buffer->buff );

        va_list args;
        va_start( args, format );
        buffer->len = vsnprintf( buffer->buff + buffer_write_index, MAX_BUFF_LEN - buffer_write_index, format, args );
        buffer->len += buffer_write_index;
        va_end( args );

        if( buffer->len >= MAX_BUFF_LEN )
            buffer->len = MAX_BUFF_LEN-1;
        buffer->buff[buffer->len++] = '\n';

        buffer->type = type;

        mtx_.Lock();
		//write_list_.push_back( buffer );
        list_add_tail( &(buffer->link_), &write_list_ );
        mtx_.Unlock();

		write_event_.set_event( 0 );
	}

    };

/*
* 所有的log在terminal方式下运行都是输出到标准IO，在daemon方式下运行输出到文件
* 分级如下：
* LOG(1)		循环写的log
*/

#define		FLOG	NLog::write_file

//for lua
#define     L_ERR(n) ( g_log->*( g_log->get_logf( LOG_TYPE_ERROR, n ) ) )
#define     L_LOG(n)	( g_log->*( g_log->get_logf( LOG_TYPE_LOG, n ) ) )
#define     L_TRACE(n) ( g_log->*( g_log->get_logf( LOG_TYPE_TRACE, n ) ) )
#define     L_ITEM_C(n) ( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_CREATE, n ) ) )
#define     L_ITEM_T(n) ( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_TRADE, n ) ) )
#define     L_ITEM_U(n) ( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_USE, n ) ) )
#define     L_GM(n) ( g_log->*( g_log->get_logf( LOG_TYPE_GM, n ) ) )
#define     L_PLAYER(n) ( g_log->*( g_log->get_logf( LOG_TYPE_PLAYER, n ) ) )
#define     L_EVENT(n) ( g_log->*( g_log->get_logf( LOG_TYPE_EVENT, n ) ) )
#define     L_LOGIN(n) ( g_log->*( g_log->get_logf( LOG_TYPE_LOGIN, n ) ) )
#define     L_SAFE(n) ( g_log->*( g_log->get_logf( LOG_TYPE_SAFE, n ) ) )
#define     L_POPO(n) ( g_log->*( g_log->get_logf( LOG_TYPE_POPO, n ) ) )
#define     L_SAVE(n) ( g_log->*( g_log->get_logf( LOG_TYPE_SAVE, n ) ) )
#define     L_PROF(n) ( g_log->*( g_log->get_logf( LOG_TYPE_PROF, n ) ) )
#define     L_TEST(n) ( g_log->*( g_log->get_logf( LOG_TYPE_TEST, n ) ) )
#define     L_SELLS(n) ( g_log->*( g_log->get_logf( LOG_TYPE_SELLS, n ) ) )
#define     L_TRIGGER(n) ( g_log->*( g_log->get_logf( LOG_TYPE_TRIGGER, n ) ) )
#define     L_INSTANCE(n) ( g_log->*( g_log->get_logf( LOG_TYPE_INSTANCE, n ) ) )
#define     L_MONEY(n) ( g_log->*( g_log->get_logf( LOG_TYPE_MONEY, n ) ) )
#define     L_ITEM_B(n) ( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_BIND, n ) ) )
#define     L_SYSLOG(n) ( g_log->*( g_log->get_logf( LOG_TYPE_SYSLOG, n ) ) )
#define     L_FIGHT(n) ( g_log->*( g_log->get_logf( LOG_TYPE_FIGHT, n ) ) )
#define     L_SDKLOG(n) ( g_log->*( g_log->get_logf( LOG_TYPE_SDKLOG, n ) ) )
#define     L_BACKUP(n) ( g_log->*( g_log->get_logf( LOG_TYPE_BACKUP, n ) ) )
#define     L_ONLINE(n) ( g_log->*( g_log->get_logf( LOG_TYPE_ONLINE, n ) ) )
#define     L_TGALOG(n) ( g_log->*( g_log->get_logf( LOG_TYPE_TGALOG, n ) ) )



#define     ERR_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_ERROR, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     LOG_2(fmt,...)	\
    ( g_log->*( g_log->get_logf( LOG_TYPE_LOG, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     TRACE_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_TRACE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     ITEM_C_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_CREATE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     ITEM_T_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_TRADE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     ITEM_U_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_USE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     GM_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_GM, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     PLAYER_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_PLAYER, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     EVENT_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_EVENT, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     LOGIN_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_LOGIN, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     SAFE_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_SAFE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     POPO_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_POPO, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     SAVE_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_SAVE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     PROF_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_PROF, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     TEST_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_TEST, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     SELLS_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_SELLS, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     TRIGGER_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_TRIGGER, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     INSTANCE_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_INSTANCE, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     MONEY_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_MONEY, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     ITEM_B_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_ITEM_BIND, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 
#define     FIGHT_2(fmt,...) \
	( g_log->*( g_log->get_logf( LOG_TYPE_FIGHT, 2 ) ) )\
	("[%s:%d] " fmt, __FILE__, __LINE__ ,##__VA_ARGS__) 

#define    ERR(n)     ERR_2
#define    LOG(n)     LOG_2
#define    TRACE(n)   TRACE_2
#define    ITEM_C(n)  ITEM_C_2
#define    ITEM_T(n)  ITEM_T_2
#define    ITEM_U(n)  ITEM_U_2
#define    GM(n)      GM_2
#define    PLAYER(n)  PLAYER_2
#define    EVENT(n)   EVENT_2
#define    LOGIN(n)   LOGIN_2
#define    SAFE(n)    SAFE_2
#define    POPO(n)    POPO_2
#define    SAVE(n)    SAVE_2
#define    PROF(n)    PROF_2
#define    TEST(n)    TEST_2
#define    SELLS(n)   SELLS_2
#define    TRIGGER(n) TRIGGER_2
#define    INSTANCE(n) INSTANCE_2
#define    MONEY(n)   MONEY_2
#define    ITEM_B(n)  ITEM_B_2
#define    FIGHT(n) FIGHT_2

#define LOG_PROCESS_TIMER()	g_log->process_timer()

#define g_log Singleton<NLog>::instance_ptr()
//extern NLog* g_log;

#endif
