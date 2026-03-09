#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef  _DEBUG
#include <cassert>
#endif

#include <string>
#include <vector>
#include <map>
#include <list>
#include <queue>
#include <stdexcept>
#include <algorithm>
#include "log.h"
#include <stdint.h>
#include "vector.h"
#include "navmesh.h"
#include "share_const.h"

#define AI_VERBOSE_PRINT
#define AI_RARE_PRINT

using std::string;
using std::vector;
using std::list;
using std::map;
using std::runtime_error;
using std::max;
using std::min;

namespace SGame
{

static const unsigned short DOORFLAG_ALLPASS = 0xffff;
static const unsigned short DOORFLAG_ALLBLOCK = 0x0;

static const float PI = 3.141592653f;
static const float THOUSAND = 1000.0f;

static const float RANGE_DIS_BUFF = 1;
static const float RANGE_ANGLE_BUFF = 10;

extern const char *AI_ROOT;

enum
{
	MAX_NUM_ID = 16,
	MAX_FILE_NAME = 256,
};

enum AI_MSG
{
	MSG_NONE,
	MSG_INIT,
	MSG_ACTIVATE,
	MSG_STEP,
	MSG_TERMINATE,
	MSG_SYNC,
	MSG_NUM,
};

enum AI_TYPE
{
	ACTION_THINK,
	ACTION_EXPLORE,
	ACTION_SWING,
	ACTION_EVASION,
	ACTION_CHASE,
	ACTION_ATTACK,
	ACTION_IDLE,
	ACTION_CHARGE,
	ACTION_ENCIRCLE,
	ACTION_SPAWN,
	ACTION_JUMP,
    ACTION_FOLLOW,
	ACTION_HOLD,
	ACTION_VARYING_CHASE,
	ACTION_SKILL_CHASE,
	ACTION_SEEK,
	ACTION_SCRIPT_SEEK,
	ACTION_RETURN,
	ACTION_ZAP,

	COND_TARGET,
	COND_MASTER,
	COND_MONSTER,
	COND_USER,
    COND_SELF, 
	COND_TARGET_ANGLE,

	COND_IN_START,
	COND_IN,
	COND_DIE_IN,
	COND_IN_END,

	COND_OUT_START,
	COND_OUT,
	COND_OUT_END,

	COND_BOARD,
	COND_BUFF,
	COND_LESS,
	COND_EQUAL,
	COND_GREAT,

	COND_POS_DIST,
	COND_AREA_OBJ,
	COND_ENEMY,
	COND_FRIEND,

	COND_STATE,
	COND_DIE,
	COND_ALL_DIE,
	COND_SUB_HP,

	COND_HURT_IN,
	COND_LAST_HP,
	COND_SKILL,
	COND_MELEE,
	COND_RANGE,
	COND_CAN_BREAK,

	RANGE_FLOAT,
	RANGE_TUPLE,
	RANGE_LIST,
	RANGE_MELEE,

	PATH_DETOUR,
	PATH_DIRECT,

	TARGET_FIRST,
	TARGET_SECOND,
	TARGET_SEEK,
	TARGET_FRONT,
	TARGET_LAST,
	COND_SECTION,		// 时间分段标记
	COND_TIME,			// 当前时间
    COND_FAR,           // 选择last中最远的
    COND_NEAR,          // 选择last中最近的
    COND_TARGET_SIDE,   // 目标所在方位
    COND_LEFT,          // 左侧
    COND_RIGHT,         // 右侧
    COND_RAGE_VALUE,
};

enum RANGE_TYPE
{
	//[180,18,12,22]
	RANGE_VIEW_ANGLE,		
	RANGE_SIGHT,			//视野角度+距离(扇形),进入则战斗
	RANGE_MOTION,			//听觉(圆形),进入则战斗			
	RANGE_CHASE,			//追击距离,超出则脱战

	RANGE_LENGTH,

	RANGE_NEAR_WALK,
	RANGE_COMMON,
	RANGE_LISTEN,
	RANGE_SENSE,
};

enum COMBAT_MODE
{
	COMBAT_NONE,
	COMBAT_ATTACK,
	COMBAT_GUARD,
	COMBAT_FOLLOW
};

enum SKILL_TGT_TYPE
{
	TGT_SKILL,
	TGT_POS_SKILL,
	POS_SKILL,
};

enum SKILL_RANGE_TYPE
{
	MELEE_SKILL,
	RANGE_SKILL,
	MELEE_RANGE_SKILL,
};

enum NODE_ID
{
	EVASION_ID = 10000,
	RETURN_ID,
	SPAWN_JUMP_ID,
	SPAWN_ID,
	SPAWN_PATROL_ID,
	SPAWN_ACT_ID,
	SCRIPT_SEEK_ID,
	SCRIPT_ACT_ID,
	EXTRA_SKILL_ID,
};

enum SELECT_AI_TARGET_TYPE
{
    SELECT_TYPE_HIGHEST_THREAT = 1, 
    SELECT_TYPE_SECOND_THREAT, 
    SELECT_TYPE_LOWEST_THREAT, 
    SELECT_TYPE_RANDOM_THREAT, 
};


typedef uint8_t ANI_TYPE;
typedef uint8_t ACT_TYPE;
typedef uint8_t TURN_TYPE;
typedef uint8_t MSG_CONDITION;
typedef uint8_t SKILL_FAIL_TYPE;
typedef uint8_t MSG_TYPE;
typedef uint8_t SPAWN_FLAG;

static const uint8_t ANI_FRONT_INDEX = SHARE_ANI_WALK - SHARE_ANI_WALK;
static const uint8_t ANI_LEFT_INDEX = SHARE_ANI_WALK_LEFT - SHARE_ANI_WALK;
static const uint8_t ANI_RIGHT_INDEX = SHARE_ANI_WALK_RIGHT - SHARE_ANI_WALK;
static const uint8_t ANI_BACK_INDEX = SHARE_ANI_WALK_BACK - SHARE_ANI_WALK;

static const float DEFAULT_BOUNDING_RADIUS = 1.2f;

struct PATROL_INFO
{
	PATROL_INFO(VECTOR3 _pos, string _anim_name, float _anim_time, int32_t _chat_id)
		:pos_(_pos), anim_name_(_anim_name), anim_time_(_anim_time), chat_id_(_chat_id)
	{
	}
	VECTOR3 pos_;
	string anim_name_;
	float anim_time_;
	int32_t chat_id_;
};

static const float CHECK_VALID_POS_RADIUS = 1.0f;

}

#endif  // __COMMON_H__
