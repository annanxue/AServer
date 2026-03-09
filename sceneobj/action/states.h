#ifndef _STATES_H_
#define _STATES_H_

#include "app_base.h"

enum {
    STATE_STAND         = 0, 
    STATE_MOVE_TO       = 1, 
    STATE_DEAD          = 2, 
    STATE_RUSH          = 3, 
    STATE_SKILLING      = 4, 
    STATE_HURT          = 5, 
    STATE_MOVE_GROUND   = 6,
    STATE_NAVIGATION    = 7,
    STATE_HURT_BACK     = 8,
    STATE_DAZED         = 9,
    STATE_HURT_FLY      = 10,
    STATE_MOVE_PERSIST  = 11,
    STATE_HURT_BACK_FLY = 12,
    STATE_HOLD          = 13,
	STATE_CINEMA        = 14,
	STATE_HURT_HORI     = 15,
	STATE_DRAG          = 16,
	STATE_REPLACE       = 17,
	STATE_LEADING		= 18,
	STATE_PULL          = 19,
	STATE_PICK          = 20,
	STATE_HURT_FLOAT    = 21,
    STATE_FREEZE        = 22,
    STATE_FEAR          = 23,
    STATE_DANCE         = 24,
    STATE_CAUGHT        = 25,
    STATE_MAX           = 26,
};

enum {    
    MSG_USER = 10, 
    MSG_HIT,                 
    MSG_MAX                 
};

enum SkillParam{
    SKILL_PARAM_SKILL_ID = 0,
    SKILL_PARAM_HJ_TIME = 1,
    SKILL_PARAM_HIT_TIME = 2,
    SKILL_PARAM_BS_TIME = 3,
    SKILL_PARAM_END_TIME = 4,
    SKILL_PARAM_TARGET_ID = 5,
    SKILL_PARAM_ANIM_SECTION = 6,
    SKILL_PARAM_IS_REQUEST = 7,
    SKILL_PARAM_FRAME_COUNTER = 8,
    SKILL_PARAM_HIT_COUNTER = 9,
    SKILL_PARAM_TARGET_X = 10,
    SKILL_PARAM_TARGET_Z = 11,
};

enum MovePersistParam {
    MOVE_PERSIST_PARAM_SKILL_ID      = 0,
    MOVE_PERSIST_PARAM_PERIOD        = 2,
    MOVE_PERSIST_PARAM_FRAME_COUNTER = 3,
    MOVE_PERSIST_PARAM_CAST_TIME     = 4,
    MOVE_PERSIST_PARAM_TOTAL_TIME    = 5,
    MOVE_PERSIST_PARAM_STAGE         = 9,   // 2:pre -> 0:idle -> 1:cast
    MOVE_PERSIST_PARAM_HIT_COUNT     = 10,
    MOVE_PERSIST_PARAM_PRE_TIME      = 11,
};

enum LeadingParam {
	LEADING_PARAM_SKILL_ID      = 0,
	LEADING_PARAM_DEC_TIME		= 1,
	LEADING_PARAM_PERIOD        = 2,
	LEADING_PARAM_FRAME_COUNTER = 3,
	LEADING_PARAM_CAST_TIME     = 4,
	LEADING_PARAM_TOTAL_TIME    = 5,
	LEADING_PARAM_TARGET_ID		= 6,
	LEADING_PARAM_RANGE			= 7,
	LEADING_PARAM_ENDWITH_DIE   = 8,
	LEADING_PARAM_IS_CANCEL     = 9,
	LEADING_PARAM_HIT_COUNT     = 10,
};

enum RushParam {
    RUSH_PARAM_SPEED             = 0,
    RUSH_PARAM_RANGE             = 1,
    RUSH_PARAM_SKILL_ID          = 2,   // negative for jump attack, positive for rush attack
    RUSH_PARAM_START_X           = 3,
    RUSH_PARAM_START_Z           = 4,
    RUSH_PARAM_SEARCH_RADIUS     = 5,
    RUSH_PARAM_APPLY_TYPE        = 6,
    RUSH_PARAM_TIME_TO_CAST_END  = 7,   // init value: length between cast hit and cast end
    RUSH_PARAM_TIME_TO_CAST_HIT  = 8,   // init value: length between cast start and cast hit
    RUSH_PARAM_STAGE_ID          = 9,   // 3: pre -> 0: moving -> 1:waiting for cast hit -> 2:waiting for cast end
    RUSH_PARAM_CAST_BS_TIME      = 10,
    RUSH_PARAM_TIME_TO_PRE_END   = 11,
};

enum DragParam {
    DRAG_PARAM_CENTER_X    = 0,
    DRAG_PARAM_CENTER_Z    = 1,
    DRAG_PARAM_SPEED       = 2,
    DRAG_PARAM_ATTACKER_ID = 3,
    DRAG_PARAM_TIME_IN_MS  = 4,
};

enum HoldParam {
    HOLD_PARAM_TIME_IN_MS = 0,
};

enum DeadType {
    DTYPE_NORMAL = 1,
    DTYPE_FLY    = 2,
    DTYPE_BACK   = 3,
    DTYPE_BOMB   = 4,
};

enum NavigationType {
    NORMAL_NAV    = 1,        //for normal navigation
    PARCHMENT_NAV = 2,        //for hero do parchment navigation, can be break by server and client on initiative
    TASK_NAV      = 3,        //for hero do task navigation, can be break by client on initiative
};

#define MAX_BITSET ( (STATE_MAX + 31) / 32 )

void init_states();

class StatesModule : public AppClassInterface
{
    public:
        bool app_class_init();    
        bool app_class_destroy(); 
};

#endif

