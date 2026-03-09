#ifndef __DEFINE_H__
#define __DEFINE_H__

/* 按照场景的大小,将world下的处理队列容量分为两类*/
#define MAX_ADDOBJS_L					20480
#define MAX_DYNAMICOBJS_L				163840
#define MAX_MODIFYLINKOBJS_L			8192
#define MAX_REPLACEOBJS_L				2048
#define MAX_DELETEOBJS_L				16384
#define MAX_ADDOBJS_S					512
#define MAX_DYNAMICOBJS_S				1024
#define MAX_MODIFYLINKOBJS_S			512
#define MAX_REPLACEOBJS_S				512
#define MAX_DELETEOBJS_S				512


//static, player, dynamic
#define	DYNAMIC_LINKTYPE				3
#define	DYNAMIC_LINKLEVEL				1
#define STATIC_LINKLEVEL				1

#define OT_OBJ      	0x00000001
#define OT_STATIC   	0x00000002
#define OT_CTRL     	0x00000004
#define OT_COMMON   	0x00000008
#define OT_ITEM     	0x00000010
#define OT_SFX      	0x00000020
#define OT_SPRITE   	0x00000040
#define OT_PLAYER   	0x00000080
#define OT_MONSTER  	0x00000100
#define OT_NPC      	0x00000200
#define OT_PET      	0x00000400
#define OT_BULLET       0x00000800
#define OT_TRIGGER      0x00001000
#define OT_MAGIC_AREA   0x00002000
#define OT_ROBOT        0x00004000
#define OT_PICK_POINT   0x00008000
#define OT_MISSILE      0x00010000

#define LANDSCAPE_SIZE					512
#define MPU								256
#define MAP_SIZE						(LANDSCAPE_SIZE / MPU)
#define MIN_STATIC_LAND_SIZE			256
#ifdef DEBUG
#define MIN_DYNAMIC_LAND_SIZE			32
#else
#define MIN_DYNAMIC_LAND_SIZE			32
#endif
#define NUM_PATCHES_PER_SIDE			4
#define PATCH_SIZE                      8

#define MAX_PATH			256
#define MAX_BUFF_LEN	    8192
#define MAX_BUFF_SIZE		500

#define MAX_CERTIFY_CODE    1024   
#define HEART_BEAT_TIMEOUT  30

#endif


