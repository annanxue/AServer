#ifndef __HTIME__
#define __HTIME__

#include <sys/time.h>
#include <time.h>

#define DEF_TM( tm ) \
		extern unsigned long tm; \
		extern struct timeval tm##s; \
		extern struct timeval tm##e;


#define ACT_DEF_TM( tm ) \
		unsigned long tm = 0; \
		struct timeval tm##s; \
		struct timeval tm##e;

DEF_TM( g_tmrun )
DEF_TM( g_tmreload )
DEF_TM( g_tmgc )
DEF_TM( g_tmprocess )
DEF_TM( g_tmreceive )

#define TM_INIT( tm )		tm = 0
#define TM_START( tm )		gettimeofday( &tm##s, NULL )
#define TM_END( tm ) \
		gettimeofday( &tm##e, NULL ); \
		tm += get_span_usec( tm##s, tm##e )


inline unsigned long get_span_usec( struct timeval &start, struct timeval &end )
{
	return 10000 * ( end.tv_sec - start.tv_sec ) + ( end.tv_usec - start.tv_usec ) / 100;
}


#endif




