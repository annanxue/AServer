#ifndef __MISC_H__
#define __MISC_H__

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include "mylist.h"

#ifndef FALSE 
	#define FALSE                0 
#endif

#ifndef TRUE 
	#define TRUE                 1
#endif

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }

#define BEFORESEND( ar, pt )	\
       Ar ar;					\
       ar << (packet_type_t)pt;

#define SEND( ar, pNetMng, idTo )								\
       {														\
               int nBufSize;									\
               char* lpBuf     = (ar).get_buffer( &nBufSize );	\
               (pNetMng)->send_msg( lpBuf, nBufSize, (idTo) );	\
       }

#ifdef __linux

unsigned int msec();

bool is_letter_or_digit( char c );

#endif

// inline char* strlcpy( char* dest, const char* src, size_t n )
// {
// 	assert(dest && src);
// 	strncpy( dest, src, n );
// 	dest[n-1] = '\0';
// 	return dest;
// }

inline void path_to_unix( char* buf, int len )
{
	int i;
	for( i = 0; i < len; ++i )
	{
		if( buf[i] == '\\' )
			buf[i] = '/';
	}
}

#define TICK_TYPE_NORMAL    0x00000000
#define TICK_TYPE_PACKET    0x00010000
#define TICK_TYPE_TIMER     0x00020000
#define TICK_TYPE_STATE     0x00040000
/*
#define TICK_PARALLEL   1
#define TICK_FSM        2
#define TICK_AI         3 
#define TICK_SERIAL     4
#define TICK_PROCESS    7
#define TICK_TIMER      8
#define TICK_LUA_GC     9
#define TICK_TOTAL      0xA 
#define TICK_ADD        0xC
#define TICK_PRO        0xD
#define TICK_MOD        0xE
#define TICK_REP        0xF
#define TICK_DEL        0x10
#define TICK_PRO_ENMITY 0x11
#define TICK_PRO_RGN    0x12

#define TICK_SERIAL_P   0x14 
#define TICK_SERIAL_P_S 0x15 
#define TICK_SERIAL_M   0x16 
*/

//下面的序号带有包换关系，用来表示调用层级
//特别注意TICK_TOTAL不包含TICK_LUA_GC,
//且TICK_FSM在单独的线程中, 包含于TICK_PARALLEL，但是由于是多线程，因此时间消耗上会重复计算，反而超出óúTICK_PARALL
#define TICK_TOTAL      0x1000
#define TICK_PROCESS    0x2000
#define TICK_ADD        0x2100
#define TICK_PRO        0x2200
#define TICK_PARALLEL   0x2210
#define TICK_FSM        0x2211
#define TICK_SERIAL     0x2220
#define TICK_MON_AI     0x2221
#define TICK_PET_AI     0x2222
#define TICK_PRO_ENMITY 0x2223
#define TICK_PRO_RGN    0x2224
#define TICK_MOD        0x2300
#define TICK_REP        0x2400
#define TICK_DEL        0x2500
#define TICK_TIMER      0x3000
#define TICK_LUA_GC     0x4000
#define TICK_LUA_MSG    0x5000
#define TICK_TOTAL_RECV     0x6000
#define TICK_TOTAL_NOTIFY   0x7000
#define TICK_MOD_DELAY      0x8000

//序列化的消耗单独编号
#define TICK_SERIAL_P   0x0001
#define TICK_SERIAL_P_S 0x0002
#define TICK_SERIAL_M   0x0003
#define TICK_SERIAL_PET 0x0004



#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))


#define rdtscl(low) \
     __asm__ __volatile__("rdtsc" : "=a" (low) : : "edx")


#define TICK( name ) \
    unsigned long long name = 0; \
    rdtscll( name );

#define TICK_L( name )	\
	unsigned long long L##name;	\
	rdtscll( L##name );


#define TICK_H( name )	\
	unsigned long long H##name;	\
	rdtscll( H##name );	\
	name += H##name - L##name;
    

#define CPUMHz	2789762000ul

#define SECM( time ) \
	( time / ( CPUMHz / 1000 ) )
	

typedef struct 
{
    int id;
    int frame;
    unsigned long long tick;
    int count;
    list_head link;

    unsigned long long t_tick;
    int t_count;

} UseTime;

void init_tick();
void mark_tick( int id, unsigned long long A, unsigned long long B );
void log_tick( int output_flag );
void track();
void log_all_tick();
int get_msec( unsigned long long tick );
int get_mm();
unsigned long long get_cpu_tick();
int str_to_time( time_t *dttime, const char *stime);
int str_to_time_by_event_type( int *dttime, const char *stime, int event_type, int *dtpara1, int *dtpara2 );
int urlencode(char const *s, int len, unsigned char* dst, int dst_len, int *new_length);
int urldecode(char *str, int len);
u_long ip2num( const char* ip );

#endif
