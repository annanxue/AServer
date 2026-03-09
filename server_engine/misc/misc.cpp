#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

//#include "game_svr.h"
#include <ctype.h>
#include "ffsys.h"
#include "misc.h"
#include <execinfo.h>
#include "mymap.hpp"
#include "mymap32.h"

#ifdef __linux
extern int g_frame;

#define LIB_YEAR ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))

#define LIB_MONTH (__DATE__ [2] == 'n' ? (__DATE__[1] == 'a' ? 0 : 5)  \
                        : __DATE__ [2] == 'b' ? 1 \
                        : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3) \
                        : __DATE__ [2] == 'y' ? 4 \
                        : __DATE__ [2] == 'l' ? 6 \
                        : __DATE__ [2] == 'g' ? 7 \
                        : __DATE__ [2] == 'p' ? 8 \
                        : __DATE__ [2] == 't' ? 9 \
                        : __DATE__ [2] == 'v' ? 10 : 11)

#define LIB_DAY ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 + (__DATE__ [5] - '0'))

#define LIB_HOUR ((__TIME__[0] - '0') * 10 + (__TIME__[1] - '0'))

#define LIB_MIN ((__TIME__[3] - '0') * 10 + (__TIME__[4] - '0'))

#define LIB_SEC ((__TIME__[6] - '0') * 10 + (__TIME__[7] - '0'))

int TIME_OUT_SECONDS_THRESHOLD = 3600 * 24 * 18;

time_t get_lib_build_time()
{
    struct tm ts;
    ts.tm_sec = LIB_SEC;
    ts.tm_min = LIB_MIN;
    ts.tm_hour = LIB_HOUR;
    ts.tm_mday = LIB_DAY;
    ts.tm_mon = LIB_MONTH;
    ts.tm_year = LIB_YEAR - 1900;
    ts.tm_isdst = -1;
    return mktime( &ts );
}

int is_lib_time_out()
{
    return false;

    time_t now = time( NULL );

    time_t old = get_lib_build_time();

    int passed = now - old;

#ifdef DEBUG
    FLOG( "./app_trace.log", "lib build time: %s %s, %u seconds passed", __DATE__, __TIME__, passed );
#endif

    if( passed < 0 )
    {
        return true;
    }
    else if( passed > TIME_OUT_SECONDS_THRESHOLD )
    {
        return true;
    }

    return false;
}

unsigned int msec()
{	
    static struct timeval tmboot = {0, 0};
    struct timezone tz;
	struct timeval tv;
    if(tmboot.tv_sec == 0) {
        gettimeofday(&tmboot,&tz);
        tmboot.tv_usec = 0;
    }
	gettimeofday(&tv,&tz);
    return (unsigned int)((tv.tv_sec - tmboot.tv_sec) * 1000 + tv.tv_usec * 0.001);
}


bool is_letter_or_digit( char c )
{
	char tmp_char = tolower( c );
	return ( ( tmp_char>='a' && tmp_char<='z' ) || ( tmp_char>='0' && tmp_char<='9' ) );
}


static MyMap32 map_tick;
static UseTime node_pool[4096];
static list_head list_tick;
static list_head free_list;

unsigned long long cpu_tick;
unsigned long long total = 0; // the time use between 2 log_tick

void init_tick()
{
    map_tick.init( 1024, 4096, "map_tick:misc.cpp" );
    INIT_LIST_HEAD( &list_tick );
    INIT_LIST_HEAD( &free_list );

    memset( node_pool, 0, sizeof(node_pool) );
    unsigned int i = 0; 
    for( i = 0; i < sizeof(node_pool)/sizeof(UseTime); ++i ) {
        list_add( &(node_pool[i].link), &free_list );
    }

    cpu_tick = get_cpu_tick();
    TRACE(2)( "[MISC](tick) TICK CPU = %llu", cpu_tick );
}

int get_msec( unsigned long long tick )
{
    return tick * 1000 / cpu_tick;
}

static Mutex lock_;

void mark_tick( int id, unsigned long long A, unsigned long long B )
{     
    AutoLock lock(&lock_);
    static int count = 0;
    UseTime* node = NULL;
    if( B > A ) {
        intptr_t value = 0;
        if( map_tick.find( id, value ) == false ) {
            count++;
            if( !list_empty( &free_list ) ) {
                list_head* pos = free_list.next;
                list_del( pos );
                node = list_entry( pos, UseTime, link );
                node->id = id;
                map_tick.set( id, (intptr_t)node );
            }
        } else {
            node = (UseTime*)value;
        }

        if( node == NULL ) return;
        if( node->frame != g_frame ) {
            node->t_tick += node->tick;
            node->t_count += node->count;

            node->frame = g_frame;
            node->tick = 0;
            node->count = 0;
        }

        node->tick += ( B - A );
        node->count += 1;
        list_del_init( &(node->link ) );
        list_add( &(node->link), &list_tick );
        total += ( B - A );
    }
}

FILE* ftick = NULL;
void log_tick( int output_flag )
{
    unsigned long long t = total;
    total = 0;
    
    if( output_flag == 0 )
    {
        //only clear total value
        return;
    }


    if( t * 10 > cpu_tick ) {
        
        time_t tm_now;
        time( &tm_now );
        struct tm tm_time;
        localtime_r(&tm_now,&tm_time); 
        //int time_len;
        char time_str[256] = { '\0' };
        //time_len = strftime( time_str, 256, "[%Y-%m-%d %H:%M:%S] ", &tm_time );
        
        //if(ftick) fprintf(ftick,  "%s[MISC](logtime total) frame = %-8d, total = %-8lu ---------------------\n", time_str, g_frame,  (u_long)((t * 1.0f / cpu_tick ) * 1000) ) ;
        PROF(2)( "%s[MISC](logtime total) frame = %-8d, total = %-8lu ---------------------\n", time_str, g_frame,  (u_long)((t * 1.0f / cpu_tick ) * 1000)  );
        UseTime* node = NULL;
        list_head* pos = NULL;
        list_for_each_safe( pos, &list_tick ) {
            node = list_entry( pos, UseTime, link );
            if( node->frame < g_frame - 1 ) return;

            unsigned long long tick = node->tick * 1000 / cpu_tick; 
            if( tick < 5 ) {
                continue;
            } else {
                int type = node->id >> 16;
                int id = ( node->id & 0x0000FFFF );
                //if(ftick) fprintf(ftick, "%s[MISC](logtime_single) id = 0x%04X-%04X, msec = %-8llu, count = %-8d, frame = %-8d \n", time_str, type, id, tick, node->count, node->frame );
                PROF(2)( "%s[MISC](logtime_single) id = 0x%04X-%04X, msec = %-8llu, count = %-8d, frame = %-8d \n", time_str, type, id, tick, node->count, node->frame );
                /*
                if( (g_gamesvr && g_gamesvr->popo_guard_ & POPO_GUARD_PROF) > 0 ) {
                    POPO(0)("[MISC](logtime_single) id = 0x%04X-%04X, msec = %-8llu, count = %-8d, frame = %-8d", type, id, tick, node->count, node->frame);
                }
                */
            }
            node->frame = -100;
        }
    }
    //if(ftick) fflush(ftick);
}

void log_all_tick()
{
    UseTime* node = NULL;
    list_head* pos = NULL;
    
    time_t tm_now;
    time( &tm_now );
    struct tm tm_time;
    localtime_r(&tm_now,&tm_time); 
    //int time_len;
    char time_str[256] = { '\0' };
    //time_len = strftime( time_str, 256, "[%Y-%m-%d %H:%M:%S] ", &tm_time );

    if(ftick) fprintf(ftick,  "%s[MISC](logtime_all) start = %-8d---------------------\n", time_str, g_frame );
    list_for_each_safe( pos, &list_tick ) {
        node = list_entry( pos, UseTime, link );
        unsigned long long tick = node->t_tick * 1000 / cpu_tick; 
        if( tick > 20 ) {
            int type = node->id >> 16;
            int id = ( node->id & 0x0000FFFF );
            double tick_percent = tick / 60000.0f * 100.0f;
            if(ftick) fprintf(ftick, "%s[MISC](logtime_all) id = 0x%04X-%04X, msec = %-8llu, percent = %%%-6.2f , count = %-8d \n", time_str, type, id, tick, tick_percent, node->t_count );
        }
        node->t_count = 0;
        node->t_tick = 0;
    }
    if(ftick) fprintf(ftick, "%s[MISC](logtime_all) end   -------------------------\n", time_str );
    if(ftick) fflush(ftick);
}


void track()
{
    /*!\todo loon*/
    return ;
    int i = 0;
    void *bt[128] = {0,};
    int bt_size = 0;
    bt_size = backtrace( bt, sizeof(bt)/sizeof(void*) );
    char** rt = backtrace_symbols( bt, sizeof(bt)/sizeof(void*) );

    TRACE(1)( "[MISC](backtrace)-------------TRACE-START----------" );
    for( i = 0; i < bt_size; ++i ) {
        TRACE(1)( "[MISC](track) BACK_TRACE : %s", rt[i] );
    }
    free( rt );
    TRACE(1)( "[MISC](backtrace)-------------TRACE-END------------" );

}


int get_mm()
{
    pid_t pid = getpid();
    char name[MAX_PATH] = {0,};
    snprintf( name, sizeof(name), "/proc/%d/status", pid );
    char buffer[4096] = {0,};
    int vm_size = 0;
    //int wm_size = 0;

    int fd = open( name, O_RDONLY);
    if( fd > 0 ) { 
        int len = read(fd, buffer, sizeof(buffer)-1);   
        close(fd);  
        buffer[len] = '\0';
        if( len > 0 ) {
            char* p = strstr( buffer, "VmSize:" );
            if( p ) { 
                int j = 0;
                int k = 0;
                char num[16] = {0,};
                bool start = false;
                for( j = 0; j < 50; ++j ) { 
                    if( p[j] >= '0' && p[j] <= '9' ) { 
                        num[k++] = p[j];
                        start = true; 
                    } else {
                        if( start == true ) { 
                            break;
                        }   
                    }   
                }   
                if( num[0] != 0 ) { 
                    vm_size = atoi( num );
                }   
            }


            /*
            p = strstr( buffer, "VmSize:" );
            if( p ) { 
                int j = 0;
                int k = 0;
                char num[16] = {0,};
                bool start = false;
                for( j = 0; j < 50; ++j ) { 
                    if( p[j] >= '0' && p[j] <= '9' ) { 
                        num[k++] = p[j];
                        start = true; 
                    } else {
                        if( start == true ) { 
                            break;
                        }   
                    }   
                }   
                if( num[0] != 0 ) { 
                    wm_size = atoi( num );
                }   
            }
            */
        }
    }  
//    return wm_size;
    return vm_size;
}

unsigned long long get_cpu_tick()
{
    char name[MAX_PATH] = {0,};
    snprintf( name, sizeof(name), "/proc/cpuinfo" );

    char buffer[4096] = {0,};
    int fd = open( name, O_RDONLY);
    if( fd > 0 ) { 
        int len = read(fd, buffer, sizeof(buffer)-1);   
        close(fd);  
        buffer[len] = '\0';
        if( len > 0 ) {
            char* p = strstr( buffer, "cpu MHz" );
            if( p ) { 
                int j = 0;
                int k = 0;
                char num[16] = {0,};
                bool start = false;
                for( j = 0; j < 50; ++j ) { 
                    if( p[j] >= '0' && p[j] <= '9' ) { 
                        num[k++] = p[j];
                        start = true; 
                    } else {
                        if( start == true ) { 
                            break;
                        }   
                    }   
                }
                unsigned int tick = atoi(num); 
                return tick * 1000000;
            }
        }
    }
    return 2000 * 1000000;
}


// time format like "2007-01-20 00:59:05"
int str_to_time( time_t *dttime, const char *stime)
{
    struct tm t = {0,};
    int ret = sscanf( stime, "%4u-%2u-%2u %2u:%2u:%2u", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec );
    if( ret != 6 )
    {
        //error
        return 1;
    }
    t.tm_mon  -= 1;
    t.tm_year -= 1900;
    if( (*dttime = mktime( &t )) == (time_t)-1 )
    {
        return 1;
    }
    return 0;
}

int str_to_time_by_event_type( int *dttime, const char *stime, int event_type, int *dtpara1, int *dtpara2 )
{
    if( !dttime )
        return 1;

    struct tm t = { 0, };
    int ret;
    int time_value;
    int para1=0, para2=0;

    switch( event_type )
    {
    case 1:
        ret = sscanf( stime, "%2u:%2u:%2u", &t.tm_hour, &t.tm_min, &t.tm_sec );
        if( ret != 3 )
        {
            return 1;
        }
        time_value = t.tm_hour*3600+t.tm_min*60+t.tm_sec;
        break;

    case 2:
        ret = sscanf( stime, "%1u %2u:%2u:%2u", &t.tm_wday, &t.tm_hour, &t.tm_min, &t.tm_sec );
        if( ret != 4 )
        {
            return 1;
        }
        time_value = t.tm_wday*86400+t.tm_hour*3600+t.tm_min*60+t.tm_sec;
        break;

    case 3:
        ret = sscanf( stime, "%2u %2u:%2u:%2u", &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec );
        if( ret != 4 )
        {
            return 1;
        }
        time_value = t.tm_mday*86400+t.tm_hour*3600+t.tm_min*60+t.tm_sec;
        break;

    case 4:
        ret = sscanf( stime, "%4u-%2u-%2u %2u:%2u:%2u", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec );
        if( ret != 6 )
        {
            return 1;
        }
        t.tm_mon  -= 1;
        t.tm_year -= 1900;
        t.tm_isdst = 0;
        time_value = mktime( &t );
        break;
    case 5:
        time_value = 0;
        break;
    case 6:
    case 7:
        ret = sscanf( stime, "%1u %1u %2u:%2u:%2u", &para1, &para2, &t.tm_hour, &t.tm_min, &t.tm_sec );
        if( ret != 5 )
        {
            return 1;
        }
        if ( dtpara1 != NULL && dtpara2 != NULL ){
            *dtpara1 = para1;
            *dtpara2 = para2;
        }
        time_value = t.tm_hour*3600+t.tm_min*60+t.tm_sec;
        break;

    default:
        return 1;
    }

    *dttime = time_value;

    return 0;
}


/** 
* @param s 需要编码的url字符串 
* @param len 需要编码的url的长度 
* @param new_length 编码后的url的长度 
* @return char * 返回编码后的url 
* @note 存储编码后的url存储在一个新审请的内存中， 
*      用完后，调用者应该释放它 
*/ 
int urlencode(char const *s, int len, unsigned char* dst, int dst_len, int *new_length) 
{ 
        if( dst_len<(3*len+1 ) ) {
            return 0;
        }

        unsigned char const *from, *end; 
        from = (unsigned char*)s; 
        end = (unsigned char*)s + len; 
        unsigned char* start, *to;
        start = dst;
        to = dst;

        unsigned char hexchars[] = "0123456789ABCDEF"; 

        while (from < end) { 
                unsigned char c = *from++; 

                if (c == ' ') { 
                        *to++ = '+'; 
                } else if ((c < '0' && c != '-' && c != '.') || 
                                  (c < 'A' && c > '9') || 
                                  (c > 'Z' && c < 'a' && c != '_') || 
                                  (c > 'z')) { 
                        to[0] = '%'; 
                        to[1] = hexchars[c >> 4]; 
                        to[2] = hexchars[c & 15]; 
                        to += 3; 
                } else { 
                        *to++ = c; 
                } 
        } 
        *to = 0; 
        if (new_length) { 
                *new_length = to - start; 
        } 
        return 1; 
} 

/** 
* @param str 需要解码的url字符串 
* @param len 需要解码的url的长度 
* @return int 返回解码后的url长度 
*/ 
int urldecode(char *str, int len) 
{ 
    char *dest = str; 
    char *data = str; 

    int value; 
    int c; 

    while (len--) { 
        if (*data == '+') { 
            *dest = ' '; 
        } 
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) 
        { 

            c = ((unsigned char *)(data+1))[0]; 
            if (isupper(c)) 
                c = tolower(c); 
            value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16; 
            c = ((unsigned char *)(data+1))[1]; 
            if (isupper(c)) 
                c = tolower(c); 
            value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10; 

            *dest = (char)value ; 
            data += 2; 
            len -= 2; 
        } else { 
            *dest = *data; 
        } 
        data++; 
        dest++; 
    } 
    *dest = '\0'; 
    return dest - str; 
}//http://www.leftworld.net [2005-07-18] 

/*! 将字符串形式的ip地址转化为数值型的值 */
u_long ip2num( const char* ip )
{
    unsigned int v1=0, v2=0, v3=0, v4=0;
    sscanf( ip, "%u.%u.%u.%u", &v1, &v2, &v3, &v4 );

    return (u_long)(v1*16777216 + v2*65536 + v3*256 + v4);
}

#endif
