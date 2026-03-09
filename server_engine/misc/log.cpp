#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <strings.h>
#include <path_misc.h>
#include <string>
#include <stdlib.h>
#include "log.h"

static const char* TERM_YELLOW_COLOR = "\x1b[33m";
static const char* TERM_RED_COLOR = "\x1b[31m";
static const char* TERM_DEFAULT_COLOR = "\x1b[0m";

//模板函数只有用到了才会实例化,并且不能用变量作为模块变量
//所以先用一个数组实例化模板函数,并存在数组中,就可以用变量访问
static NLog::logapi_t api_array[MAX_LOG_TYPE] = 
{
	&NLog::write_log<LOG_TYPE_ERROR>,
	&NLog::write_log<LOG_TYPE_LOG>,
	&NLog::write_log<LOG_TYPE_TRACE>,
	&NLog::write_log<LOG_TYPE_ITEM_CREATE>,
	&NLog::write_log<LOG_TYPE_ITEM_TRADE>,
	&NLog::write_log<LOG_TYPE_ITEM_USE>,
	&NLog::write_log<LOG_TYPE_GM>,
	&NLog::write_log<LOG_TYPE_PLAYER>,
	&NLog::write_log<LOG_TYPE_EVENT>,
	&NLog::write_log<LOG_TYPE_LOGIN>,
	&NLog::write_log<LOG_TYPE_SAFE>,
	&NLog::write_log<LOG_TYPE_POPO>,
	&NLog::write_log<LOG_TYPE_SAVE>,
	&NLog::write_log<LOG_TYPE_PROF>,
	&NLog::write_log<LOG_TYPE_TEST>,
    &NLog::write_log<LOG_TYPE_SELLS>,
    &NLog::write_log<LOG_TYPE_TRIGGER>,
    &NLog::write_log<LOG_TYPE_INSTANCE>,
    &NLog::write_log<LOG_TYPE_MONEY>,
    &NLog::write_log<LOG_TYPE_ITEM_BIND>,
    &NLog::write_log<LOG_TYPE_SYSLOG>,
    &NLog::write_log<LOG_TYPE_FIGHT>,
    &NLog::write_log<LOG_TYPE_SDKLOG>,
    &NLog::write_log<LOG_TYPE_BACKUP>,
    &NLog::write_log<LOG_TYPE_ONLINE>,
    &NLog::write_log<LOG_TYPE_TGALOG>,
};

static NLog::logapi_t api_table[MAX_LOG_TYPE][MAX_LOG_LEVEL];

NLog::NLog()
{
    thread_name_ = "NLog"; 
	INIT_LIST_HEAD( &write_list_ );
	time_len_ = 0;
	bzero( time_str_, sizeof( time_str_ ) );
    is_syslog_open_ = false;
    syslog_len_ = 0;
	bzero( syslog_str_, sizeof( syslog_str_ ) );
    date_year_ = 0;
    date_mon_ = 0;
    date_mday_ = 0;
    date_hour_ = 0;

	olog_fn_ = 0;
	olog_ln_ = 0;
	otrace_fn_ = 0;
	otrace_ln_ = 0;
	daemon_mode_ = 0;
	bzero( log_path_, sizeof( log_path_ ) );
	bzero( log_, sizeof( log_ ) );
	bzero(init_log_path_, sizeof(init_log_path_));
	bzero(temp_time_str_, sizeof(temp_time_str_));

	otrace_ = NULL;
	stdi_ = NULL;
	stdo_ = NULL;
	stde_ = NULL;

	//初始化日志API表.初始情况下 3,4级log不写
	for ( int i = 0; i < MAX_LOG_TYPE; i++ )
	{
		api_table[i][0] = api_array[i];
		api_table[i][1] = api_array[i];
		api_table[i][2] = api_array[i];
		api_table[i][3] = &NLog::write_null;
		api_table[i][4] = &NLog::write_null;
	}

	process_timer();
}


NLog::~NLog()
{
	shutdown();
}

FILE* NLog::open_log_file( const char *name )
{
	char filename[MAX_PATH] = { 0, };
	//日志文件按时间作为名字保存
	if (use_old_log_version_) {
		snprintf(filename, MAX_PATH, "%s_%s.log", log_path_, name);
	}
	else {
		snprintf(filename, MAX_PATH, "%s/%s.log", log_path_, name);
	}
	return fopen( filename, "aw+" );
}

void NLog::change_color( int _type, LogBuffer* _buffer )
{
    switch( _type )
    {
        case LOG_TYPE_ERROR:
            _buffer->color = TERM_RED_COLOR;
            break;
        case LOG_TYPE_SAFE:
            _buffer->color = TERM_YELLOW_COLOR;
            break;
        default:
            _buffer->color = TERM_DEFAULT_COLOR;
            break;
    }
}


bool NLog::execute_shell_cmd( const char* cmd ) {
	int ret = system(cmd);
	if (ret == 0) {
		return true; 
	}
	else {
		ERR(2)("[LOG] migrate_log_file system error:[%s] ret:[%d] => %s ", strerror(errno), ret, cmd);
		if (WIFEXITED(ret))
			ERR(2)("SHELL FAIL SHELL STATUS:%d ", WEXITSTATUS(ret));
		else
			ERR(2)("SHELL EXIT  SHELL STATUS:%d ", WEXITSTATUS(ret));
	}

	return false;
}
void NLog::migrate_log_file() {
	if (strlen(log_path_) <= 0 || strlen(temp_time_str_) <= 0 )
		return;
	std::string  path = log_path_;
	std::string dir, file_prefix;
	std::string::size_type find_idx = path.find_last_of("/");
	if (find_idx != std::string::npos) {
		// log目录
		dir = path.substr(0, find_idx);
		// 文件前缀
		file_prefix = path.substr(find_idx + 1);
	}


	time_t tm_now;
	if (time(&tm_now) == -1)
	{
		ERR(2)("[LOG] migrate_log_file get time now is err. [%s]", strerror(errno));
		return;
	}
	
	char		target_dir[MAX_PATH] = { 0 };
	snprintf(target_dir, sizeof(target_dir), "%s/%s/", dir.c_str(), temp_time_str_);
	// 在log目录下创建当天日期目录
	if (create_dir(target_dir) != 0)
		return;

	char			cmd[MAX_PATH] = { 0 };
	snprintf(cmd, sizeof(cmd), "mv %s/%s_*.log %s", dir.c_str(), file_prefix.c_str(), target_dir);

	// 执行shell脚本转移文件
	if (execute_shell_cmd(cmd)) {
		// 成功  
		reset_log_file_handle();
		LOG(2)("migrate_log_file success ");
	}
	else {
		return; 
	}

	// 更改文件名
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "cd %s ; for file in `ls *.log`; do mv $file ${file}_%s;done" , target_dir , temp_time_str_);
	if (execute_shell_cmd(cmd)) {
		LOG(2)("change file name success ");
	}

	struct tm tm_time;
	localtime_r(&tm_now, &tm_time);
	bzero(temp_time_str_, sizeof(temp_time_str_));
	strftime(temp_time_str_, MAX_PATH, "%Y%m%d", &tm_time);

}


int NLog::reset_log_file_handle() {
	assert(strlen(init_log_path_) > 0);

	if ( !use_old_log_version_ ) {
		time_t tm_now;
		if (time(&tm_now) == -1)
		{
			ERR(2)("[LOG] reset_log_file_handle get time now is err. [%s]", strerror(errno));
			return -1;
		}
		char		temp_time_str_[256] = { 0 };
		struct tm tm_time;
		localtime_r(&tm_now, &tm_time);   // Convert time to struct tm form 
		strftime(temp_time_str_, 256, "%Y-%m-%d", &tm_time);

		char			real_log_path[MAX_PATH];
		snprintf(real_log_path, sizeof(real_log_path), "%s/%s/", init_log_path_, temp_time_str_);
		strncpy(log_path_, real_log_path, sizeof(log_path_));
	}
	else {
		strncpy(log_path_, init_log_path_, sizeof(log_path_));
	}

	if (create_dir(log_path_) != 0)
		return -1;

	olog_ = NULL;
	otrace_ = NULL;
	stdi_ = open_log_file("stdin");
	stdo_ = open_log_file("stdout");
	stde_ = open_log_file("stderr");

	log_[LOG_TYPE_ERROR] = open_log_file("error");
	//log_[ LOG_TYPE_LOG ] = open_log_file( "log" );
	log_[LOG_TYPE_LOGIN] = open_log_file("login");
	log_[LOG_TYPE_SAFE] = open_log_file("safe");
	log_[LOG_TYPE_POPO] = open_log_file("popo");
	log_[LOG_TYPE_ITEM_CREATE] = open_log_file("item_create");
	log_[LOG_TYPE_ITEM_TRADE] = open_log_file("item_trade");
	log_[LOG_TYPE_ITEM_USE] = open_log_file("item_use");
	log_[LOG_TYPE_GM] = open_log_file("gm");
	log_[LOG_TYPE_PLAYER] = open_log_file("player");
	log_[LOG_TYPE_EVENT] = open_log_file("event");
	log_[LOG_TYPE_SAVE] = open_log_file("save");
	log_[LOG_TYPE_PROF] = open_log_file("prof");
	log_[LOG_TYPE_TEST] = open_log_file("test");
	log_[LOG_TYPE_SELLS] = open_log_file("sells");
	log_[LOG_TYPE_TRIGGER] = open_log_file("trigger");
	log_[LOG_TYPE_INSTANCE] = open_log_file("instance");
	log_[LOG_TYPE_MONEY] = open_log_file("money");
	log_[LOG_TYPE_ITEM_BIND] = open_log_file("item_bind");
	log_[LOG_TYPE_SYSLOG] = open_log_file("syslog_pipe");
	log_[LOG_TYPE_FIGHT] = open_log_file("fight");
	log_[LOG_TYPE_SDKLOG] = open_log_file("sdklog_pipe");
	log_[LOG_TYPE_BACKUP] = open_log_file("backup");
	log_[LOG_TYPE_ONLINE] = open_log_file("online");
	log_[LOG_TYPE_TGALOG] = open_log_file("tga_pipe");

	olog_fn_ = 0;
	olog_ln_ = 0;
	otrace_fn_ = 0;
	otrace_ln_ = 0;
	check_olog();
	check_otrace();
	if (!olog_ || !otrace_ || !stdi_ || !stdo_ || !stde_)
	{
		fprintf(stdout, "log init failed, olog_ %x, otrace_ %x, stdi_ %x, stdo_ %x, stde_ %x\n",
			(unsigned int)olog_, (unsigned int)otrace_, (unsigned int)stdi_, (unsigned int)stdo_, (unsigned int)stde_);
		shutdown();
		return -1;
	}

	return 0; 
}

int NLog::startup( const char* _log_path, int _daemon_mode, int _use_syslog, const char* _proc_flag, char* _syslog_list , bool _use_old_log_version /*= false*/ )
{
	if ( ! _log_path )
		return -1;

	time_t tm_now;
	if (time(&tm_now) == -1)
	{
		ERR(2)("[LOG] startup get time now is err. [%s]", strerror(errno));
		return -1 ;
	}

	
	use_old_log_version_ =  true /*_use_old_log_version*/;

	strncpy(init_log_path_, _log_path, sizeof(init_log_path_));

	struct tm tm_time;
	localtime_r(&tm_now, &tm_time);
	bzero(temp_time_str_, sizeof(temp_time_str_));
	strftime(temp_time_str_, MAX_PATH, "%Y%m%d", &tm_time);

	if (reset_log_file_handle() != 0)
		return -1;

	daemon_mode_ = _daemon_mode;
    strlcpy( proc_flag_, _proc_flag, FILE_FLAG_LEN );
    openlog( "", LOG_CONS, LOG_LOCAL0 );

	if ( start() < 0 )
		return -1;

	return 0;
} 

void NLog::shutdown()
{
    if( !active_ )
    {
        return;
    }

    active_ = FALSE;
	write_event_.set_event( 0 );

    this->join();
    
    mtx_.Lock();
    list_head* pos;
    LogBuffer *buffer = NULL;
    list_for_each_safe( pos, &write_list_ )
    {
        buffer = list_entry( pos, LogBuffer, link_ );
        list_del_init( pos );
        SAFE_DELETE( buffer );
    }
	INIT_LIST_HEAD( &write_list_ );
    mtx_.Unlock();


	if( olog_ ){
		fclose( olog_ );
		olog_ = NULL;
	}

	if (otrace_) {
		fclose(otrace_);
		otrace_ = NULL;
	}

	if( stdi_ ){
		fclose( stdi_ );
		stdi_ = NULL;
	}
	if( stdo_ ){ 
		fclose( stdo_ );
		stdo_ = NULL;
	}
	if( stde_ ){
		fclose( stde_ );
		stde_ = NULL;
	}

	for ( int i = 0; i < MAX_LOG_TYPE; i++ )
	{
		if ( log_[i] )
		{
			fclose( log_[i] );
			log_[i] = NULL;
		}
	}
    closelog();
}

void NLog::process_timer()
{
	time_t tm_now;
	if( time( &tm_now ) == -1 )
    {
        ERR(2)( "[LOG] process_timer get time now is err. [%s]", strerror(errno) );
        return;
    }
	struct tm tm_time;
    localtime_r( &tm_now, &tm_time );   // Convert time to struct tm form 
    mtx_.Lock();
	time_len_ = strftime( time_str_, 256, "[%Y-%m-%d %H:%M:%S] ", &tm_time );
    if(is_syslog_open_)
    {
        syslog_len_ = strftime( syslog_str_, 256, "%Y/%m/%d", &tm_time );
        process_syslog_open( tm_time.tm_year, tm_time.tm_mon, tm_time.tm_mday, tm_time.tm_hour );
    }
    mtx_.Unlock();
}

void NLog::write_to_stdout_on_non_daemon( LogBuffer* _buffer )
{
    if ( !daemon_mode_ )
    {
        fwrite( _buffer->color, sizeof(char), strlen( _buffer->color ), stdout );
        fwrite( _buffer->buff, sizeof(char), _buffer->len, stdout );
        // 恢复默认颜色，不然服务器马上停机的话，终端颜色就会被修改
        fwrite( TERM_DEFAULT_COLOR, sizeof(char), strlen( TERM_DEFAULT_COLOR ), stdout );
        fflush( stdout );
    }
}

void NLog::run()
{
	while ( active_ )
	{
		write_event_.wait_event( 0 );

		if ( active_ == FALSE )
        {
			break;
        }

		do
		{
			LogBuffer *buffer = NULL;

            mtx_.Lock();
            if ( !list_empty( &write_list_ ) ) {
                list_head* pos = write_list_.next;
                buffer = list_entry(pos, struct LogBuffer, link_); 
                list_del( pos );   
            }
            mtx_.Unlock();
			
			if ( NULL == buffer )
				break;
			
			switch ( buffer->type )
			{
            case LOG_TYPE_LOG:
				check_olog();
				//fwrite( time_str_, sizeof(char), time_len_, olog_ );
				fwrite( buffer->buff, buffer->len, sizeof(char), olog_ );
				fflush( olog_ );
                write_to_stdout_on_non_daemon( buffer );
				break;
			case LOG_TYPE_TRACE:
				check_otrace();
				//fwrite( time_str_, sizeof(char), time_len_, otrace_ );
				fwrite(buffer->buff, buffer->len, sizeof(char), otrace_);
				fflush(otrace_);
				write_to_stdout_on_non_daemon(buffer);
				break;
            default:
                FILE *file = log_[buffer->type];
                //fwrite( time_str_, sizeof(char), time_len_, file );
                fwrite( buffer->buff, buffer->len, sizeof(char), file );
                fflush( file );
                write_to_stdout_on_non_daemon( buffer );

                break;
            }

			SAFE_DELETE( buffer );
		} while ( !list_empty( &write_list_ ) );
	}
}

void NLog::check_olog()
{
	FILE* new_ofile = NULL;

	if( olog_ln_++ >= MAX_LINE_NUM || olog_ == NULL )
	{
		olog_ln_ = 1;
		olog_fn_ = ( olog_fn_ + 1 > MAX_FILE_NUM ) ? 0 : (olog_fn_ + 1);
		char filename[MAX_PATH] = { 0, };
		if (use_old_log_version_) {
			snprintf(filename, sizeof(filename), "%s_log_%03d.log", log_path_, olog_fn_);
		}
		else {
			snprintf(filename, sizeof(filename), "%s/log_%03d.log", log_path_, olog_fn_);
		}
		new_ofile = fopen( filename, "w+" );
	}
 	if( new_ofile )
	{ 
		if( olog_ )		fclose( olog_ );
		olog_ = new_ofile;
	}
}

void NLog::check_otrace()
{
	FILE* new_ofile = NULL;

	if (otrace_ln_++ >= MAX_LINE_NUM || otrace_ == NULL)
	{
		otrace_ln_ = 1;
		otrace_fn_ = (otrace_fn_ + 1 > MAX_FILE_NUM) ? 0 : (otrace_fn_ + 1);
		char filename[MAX_PATH] = { 0, };
		if (use_old_log_version_) {
			snprintf(filename, sizeof(filename), "%s_trace_%03d.log", log_path_, otrace_fn_);
		}
		else {
			snprintf(filename, sizeof(filename), "%s/trace_%03d.log", log_path_, otrace_fn_);
		}
		new_ofile = fopen(filename, "w+");
	}
	if (new_ofile)
	{
		if (otrace_)		fclose(otrace_);
		otrace_ = new_ofile;
	}
}

void NLog::write_null( const char* format, ... )
{
}

NLog::logapi_t NLog::get_logf( unsigned int _i, unsigned int _n )
{
	if ( ( _i < MAX_LOG_TYPE ) && ( _n < MAX_LOG_LEVEL ) )
		return api_table[_i][_n];
	else
		return &NLog::write_null;
}

void NLog::set_level(int _type, int _level)
{
	if ( ( _type >= 0 ) && ( _type < MAX_LOG_TYPE ) &&
	     ( _level >= 0 ) && ( _level < MAX_LOG_LEVEL) )
	{
		for ( int i = 0; i < MAX_LOG_LEVEL; i++ )
		{
			if ( i > _level )
				api_table[_type][i] = &NLog::write_null;
			else
				api_table[_type][i] = api_array[_type];
		}
	}
	else
	{
		write_log<LOG_TYPE_ERROR>( "[LOG](set level) set level error: type %d level %d", _type, _level );
	}
}

void NLog::write_file( const char* _filename, const char* format, ... )
{
	char buff[MAX_BUFF_LEN];

	va_list args; 
	va_start( args, format ); 
	int len = vsnprintf( buff, MAX_BUFF_LEN, format, args );
	va_end( args );

	if( len >= MAX_BUFF_LEN )
		len = MAX_BUFF_LEN-1;
	buff[len++] = '\n';

	FILE* file = fopen( _filename, "aw+" );
	if( !file )	
		return;
	fwrite( buff, len, sizeof(char), file );
	fclose( file );	
}

void NLog::process_syslog_open( int year, int mon, int mday, int hour)
{
    if( date_year_ == year
            &&date_mon_ == mon
            &&date_mday_ == mday
            &&date_hour_ == hour )
    {
        return;
    }

    date_year_ = year;
    date_mon_ = mon;
    date_mday_ = mday;
    date_hour_ = hour;
    if( log_[ LOG_TYPE_SYSLOG ] != NULL )
    {
		fclose( log_[ LOG_TYPE_SYSLOG ] );
    }

    char syslog_name[MAX_PATH] = { 0, };
    snprintf( syslog_name, sizeof(syslog_name), "%s_syslog/%s/%02d.log", log_path_, syslog_str_, date_hour_);
    int ret = create_dir(syslog_name);
    if( ret != 0 )
    {
        log_[ LOG_TYPE_SYSLOG ] = open_log_file( "syslog_pipe" );
        return;
    }
    log_[ LOG_TYPE_SYSLOG ] = fopen( syslog_name, "aw+" );
}



