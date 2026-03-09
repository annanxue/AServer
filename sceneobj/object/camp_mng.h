#ifndef __CAMP_MNG_H__
#define __CAMP_MNG_H__

#include "singleton.h"
#include "lua_server.h"
#include "lunar.h"
#include "spirit.h"

const int CAMP_TABLE_LEN_MAX = 31;

class CampMng
{
    public:
        CampMng();
    public:
        int  get_relation( int _camp1, int _camp2 );
        bool  is_friend( Spirit* _spirit1, Spirit* _spirit2 );
        bool  is_enemy( Spirit* _spirit1, Spirit* _spirit2 );
    private:
        int  relations_[CAMP_TABLE_LEN_MAX][CAMP_TABLE_LEN_MAX];
    public:
	    static const char className[];
	    static Lunar<CampMng>::RegType methods[];
	    static const bool gc_delete_;
    public:
        int  c_read_cfg( lua_State* _state );
        int  c_get_relation( lua_State* _state );
};

#define g_campmng ( Singleton<CampMng>::instance_ptr() )

#define LUNAR_CAMPMNG_METHODS	\
    method( CampMng, c_read_cfg ),\
    method( CampMng, c_get_relation )

#endif

