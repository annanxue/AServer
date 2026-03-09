#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>

#include "app_fx.h"
#include "autolock.h"
#include "log.h"

int32_t set_coresize()
{
	struct rlimit rlim_core;

	rlim_core.rlim_cur = RLIM_INFINITY;	// Soft limit
	rlim_core.rlim_max = RLIM_INFINITY;	// Hard limit (ceiling for rlim_cur)
	return setrlimit(RLIMIT_CORE, &rlim_core);
}

int32_t daemon_init(void)
{
	pid_t pid;
    
	if ((pid = fork()) != 0)
    {
		exit(0);
    }

	setsid();
	//signal( SIGHUP, SIG_IGN );

	if ((pid = fork()) != 0)
    {
		exit(0);
    }

	umask(0);

	for (int32_t i = 0; i < 3; i++) close(i);

	return 0;
}

int32_t write_pid( char* _file_name )
{
   pid_t cur_pid = getpid();
   FILE* lfile = NULL;
   lfile = fopen( _file_name, "w" );
   if ( lfile == NULL ) {
       return -1;
   }

   fprintf( lfile, "%d", (int32_t)cur_pid );
   fflush( lfile );
   fclose( lfile );
   chmod( _file_name, S_IRUSR | S_IWUSR );

   return 0;
}

int32_t read_pid( char* _file_name )
{
   FILE* lfile = NULL;
   lfile = fopen( _file_name, "r" );
   if ( lfile == NULL ) {
       return -1;
   }

   pid_t cur_pid;
   fscanf( lfile, "%d", &cur_pid );
   fclose( lfile );

   return cur_pid;
}

int32_t pre_main( int32_t argc, char* argv[], char* _cfg_filename )
{
	int32_t ch;
	char* strvar = NULL;
	opterr =0;  // 防止getopt函数打印自己的错误信息

	// 开起进程,并判断是否以精灵进程模式启动
	while ((ch = getopt( argc, argv, "c:" )) != EOF)
	{
		switch (ch)
		{
		case 'c':
			strvar = optarg;
			ff_snprintf(_cfg_filename, MAX_FILENAME, "%s", strvar );
            break;

		default:
            printf("Usage : \n    -c filename: start with config file.\n ");
			return -1;
		}
	}

	if( set_coresize() < 0 )
	{
		FLOG( (char *)"./error.log", (char *)"Failed to set corefile size to unlimited! [%s]\n", strerror(errno) );
		return -1;
	}
	
	return 0;
}
