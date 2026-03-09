#ifndef __MSG_TYPE_H__
#define __MSG_TYPE_H__


#include <sys/types.h>

typedef unsigned short			packet_type_t;

#define PACKET_TYPE_SIZE		( sizeof(packet_type_t) )

#define PT_DISCONNECT					0x0001
#define PT_CONNECT						0x0002

#define PT_PACKET_MAX                   0xffff

#endif
