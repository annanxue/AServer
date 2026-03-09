
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "state.h"
#include "player_mng.h"
#include "timer.h"

//#include "spirit.h"

#if 0
#ifdef WIN32
#include <windows.h>
    inline void TICK(unsigned long long &a)
    {
        ULARGE_INTEGER z;
        _asm{ push eax };
        _asm{ push edx };
        _asm{ xor eax, eax};
        _asm{ _emit 0x0f};
        _asm{ _emit 0x31};
        _asm{ mov z.HighPart,edx};
        _asm{ mov z.LowPart,eax};
        _asm{ pop edx };
        _asm{ pop eax };
        a = *(unsigned long long*)&z;
    }
#else
#include <unistd.h>
#define TICK(val) \
        __asm__ __volatile__("rdtsc" : "=A" (val))
#endif

#endif


StateDesc State::v_state[ STATE_MAX ]; 

inline int32_t find_next_set_bit(uint32_t *_addr, uint32_t _size, int32_t _offset)
{
    uint32_t * p = _addr + (_offset >> 5); 
    uint32_t bit = _offset & 31, t;

    if( bit ) { 
        t = (*p) & ( 0xFFFFFFFF << bit );
        if(t) {
            return int32_t( __builtin_ctz(t) + (( p - _addr ) << 5) );
        }   
        p++;
    }   
        
    for(;p < ( (_size+31) >> 5 ) + _addr; ++p) {
        if( *p ) { 
            return int32_t( __builtin_ctz(*p) + (( p - _addr ) << 5) );
        }   
    }   
    return _size;
}

inline int find_next_set_bit3(unsigned int *addr, unsigned int size, int offset)
{
    static int find_t[256] =
    {
           -1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
            4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };

    unsigned int * p = addr + (offset >> 5);
    unsigned int bit = offset & 31, j, t;

    if( bit ) { 
        t = (*p) & ( 0xFFFFFFFF << bit );
        if(t) {
            for( j = 0; j < 4; ++j ) {
                if( ((unsigned char*)&t)[j] ) {
                    return int(find_t[ ((unsigned char*)&t)[j] ] + ( ( p - addr ) << 5 ) + ( j << 3 ));
                }
            }
        }
        p++;
    }
    
    for(;p < ( (size+31) >> 5 ) + addr; ++p) {
        if( *p ) {
            for( j = 0; j < 4; ++j ) {
                if( ((unsigned char*)p)[j] ) {
                    return int(find_t[ ((unsigned char*)p)[j] ] + ( ( p - addr ) << 5 ) + ( j << 3 ));
                }
            }
        }
    }
    return size;
}


static inline int find_next_set_bit2(unsigned int *addr, unsigned int size, int offset)
{
    static unsigned char find_t[256] =
	{
	    9, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
		4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
	};

	unsigned int *ptr(addr);
	unsigned char *p(0);
	
	if( offset > 0 )
	{
		unsigned int shift(offset >> 5);
		ptr += shift;
		//align_offset = shift << 5;
		//µ¥Î»offset
		unsigned int mask(ptr[0]);
		mask &= (0xffffffff << (offset & 31));
		if(mask)
		{
			p = (unsigned char*)&mask;
			if( p[0] )
				return int(find_t[p[0]] + ( ( ptr - addr ) << 5 ));
			else if( p[1] )
				return int(find_t[p[1]] + ( ( ptr - addr ) << 5 ) + 8);
			else if( p[2] )
				return int(find_t[p[2]] + ( ( ptr - addr ) << 5 ) + 16);
			else if( p[3] )
				return int(find_t[p[3]] + ( ( ptr - addr ) << 5 ) + 24);
		}
		++ptr;
	}
	for(; ptr; ++ptr)
	{
		if(ptr[0])
		{
			p = (unsigned char*)&ptr[0];
			if( p[0] )
				return int(find_t[p[0]] + ( ( ptr - addr ) << 5 ));
			else if( p[1] )
				return int(find_t[p[1]] + ( ( ptr - addr ) << 5 ) + 8);
			else if( p[2] )
				return int(find_t[p[2]] + ( ( ptr - addr ) << 5 ) + 16);
			else if( p[3] )
				return int(find_t[p[3]] + ( ( ptr - addr ) << 5 ) + 24);
		}
	}
    return size;
}

int State::msg_retry_max = 0;

State::State()
{
    memset(&run_set, 0, sizeof(run_set));
    memset(&pause_set, 0, sizeof(pause_set));
    memset(data, 0, sizeof(data));

    top = STATE_MAX;
    qhead = qtail = 0;
    msg_tm = 0;
}

State::~State()
{

}

int State::del_state( unsigned int index )
{
    if(index >= 0 && index < STATE_MAX) {
        if(test_bit(index, run_set)) {
            clear_bit(index, run_set); 
            if(v_state[index].handle) {
                v_state[index].handle(this, index, MSG_DESTROY, 0, 0, 0, 0);
            }
            return RT_OK;
        }
    }
    return RT_ER;
}

int State::del_state_from_msg_queue( unsigned int index )
{
    int head = qhead;
    int tail = qtail;
    bool is_found = false;
    while( head < tail )
    {
        int idx = head % MAX_MESSAGE;
        int msg = (int)msg_queue[idx][0];
        int msgid = msg & 0x000000FF;
        unsigned int state = (msg >> 8) & 0x0000FFFF;
        if (state == index && msgid == SYS_SET_STATE)
        {
            msg_queue[idx][0] = SYS_DEL_STATE + STATE_MAX * 256;
            is_found = true;
        }
        head++;
    }
    return is_found ? RT_OK : RT_ER;
}

int State::is_state( unsigned int index )
{
    if(index >= 0 && index < STATE_MAX) {
        return test_bit(index, run_set);
    }
    return 0;
}

void State::process()
{
    unsigned int i = 0;
    int index = -1;
    while((index = find_next_set_bit(run_set, (sizeof(run_set) << 3), index + 1)) < (int)(sizeof(run_set) << 3)) {
        if(v_state[index].handle) {
            if(!test_bit(index, pause_set)) {
                TICK(A);
                v_state[index].handle(this, index, MSG_UPDATE, 0, 0, 0, 0);
                TICK(B);
                mark_tick( TICK_TYPE_STATE+( index << 8 )+MSG_UPDATE, A, B );
            } else {
                clear_bit(index, pause_set);
            }
        }
    }

    while(qhead < qtail) {
        index = qhead % MAX_MESSAGE;
        int rt = do_send_msg(msg_queue[index][0], msg_queue[index][1], msg_queue[index][2], msg_queue[index][3], msg_queue[index][4]);
        int *p = msg_queue[index];
        if(rt == RT_DE)
        {
            msg_tm++;
            if(msg_tm > msg_retry_max) {
                ERR(0)( "[STATE] state index: %d, retry %d times, reach retry max: %d", index, msg_tm, msg_retry_max );
                qhead++;
                msg_tm = 0;
            } else {
                break;
            }
        }else {
            TRACE(0)( "PUSHMSG msg = 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, head = %-8d, tail = %-8d", p[0], p[1], p[2], p[3], p[4], qhead, qtail );
            msg_tm = 0;
            qhead++;
        }
    } 

    index = STATE_STAND;

    for(i = 0; i < MAX_BITSET; ++i) {
        if(v_state[index].deny[i] & run_set[i]) break;
        if(v_state[index].dely[i] & run_set[i]) break;
        if(v_state[index].swap[i] & run_set[i]) break;
    }

    if(i >= MAX_BITSET) {
        set_bit(index, run_set);
    }
}

int State::post_msg( int msg, int p0, int p1, int p2, int p3 )
{
    if(qhead == qtail) {
        int rt = do_send_msg(msg, p0, p1, p2, p3);
        if(rt != RT_DE) {
            //LOG(0)( "POSTMSG msg = 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X", msg, p0, p1, p2, p3 );
            return rt; 
        }
    } 

    if(qtail - qhead < MAX_MESSAGE) {
        int index = qtail % MAX_MESSAGE;
        msg_queue[index][0] = msg;
        msg_queue[index][1] = p0;
        msg_queue[index][2] = p1;
        msg_queue[index][3] = p2;
        msg_queue[index][4] = p3;
        qtail++;
        return RT_DE;
    } else {
        return RT_ER; 
    }
}

int State::send_msg( int msg, int p0, int p1, int p2, int p3 )
{
    return do_send_msg(msg, p0, p1, p2, p3);
}

int State::do_send_msg( int msg, int p0, int p1, int p2, int p3 )
{
    TICK(A)

    int msgid = msg & 0x000000FF;
    int state = (msg >> 8) & 0x0000FFFF;

    if(msgid == SYS_SET_STATE) {
        if(state >= 0 && state < STATE_MAX){
            StateDesc *desc = &v_state[state];
            unsigned int i;
            for(i = 0; i < MAX_BITSET; ++i) {
                if(desc->deny[i] & run_set[i]) {
                    return RT_ER;
                }
            }

            for(i = 0; i < MAX_BITSET; ++i) {
                if(desc->dely[i] & run_set[i]) {
                    return RT_DE;
                }
            }

            int index = -1;
            while((index = (int)find_next_set_bit(run_set, (sizeof(run_set) << 3), index + 1)) < (int)(sizeof(run_set) << 3)) {
                if(test_bit(index, desc->swap)) {
                    //del_state(index);
                    clear_bit(index, run_set); 
                    if(v_state[index].handle) {
                        v_state[index].handle(this, index, MSG_DESTROY, 1, 0, 0, 0); // means force delete state
                    }
                }
            }

            set_bit(state, run_set);
            if(v_state[state].handle) {
                v_state[state].handle(this, state, MSG_INITIAL, p0, p1, p2, p3);
            } else {
                data[state][0] = p0;
                data[state][1] = p1;
                data[state][2] = p2;
                data[state][3] = p3;
            }
        }
    } else if(msgid == SYS_DEL_STATE) {
        if(state >= 0 && state < STATE_MAX) {
            del_state(state);  
        }
    } else if(msgid == MSG_SETDATA) {
        if(state >= 0 && state < STATE_MAX) {
            if(p0 >= 0 && p0 < MAX_PARAM) {
                set_param(state, p0, p1);
            }
        }   
    } else if(msgid == MSG_RESET) {
        memset(run_set, 0, sizeof(run_set));
        qhead = qtail = 0;
    } else {
        if(state >= 0 && state < STATE_MAX) {
            //int rt = 0;
            if(test_bit(state, run_set)) {
                if(v_state[state].handle) { 
                    /*rt = */v_state[state].handle(this, state, msgid, p0, p1, p2, p3);
                }
            } 
        }
    }
    TICK(B)
    mark_tick( TICK_TYPE_STATE+( state<<8 )+msgid, A, B );
    return RT_OK;
}


void State::pause( unsigned int index )
{
    if(index > 0 && index < STATE_MAX) {
        set_bit(index, pause_set);
    }
}

int State::can_be_state( unsigned int index )
{
    if(index > 0 && index < STATE_MAX) {
        StateDesc *desc = &v_state[index];
        unsigned int i;
        for(i = 0; i < MAX_BITSET; ++i) {
            if(desc->deny[i] & run_set[i]) {
                return RT_ER;
            }
        }

        int head = qhead;
        int tail = qtail;
        while(head < tail) 
        {
            int idx = head % MAX_MESSAGE;
            int post_state = msg_queue[idx][0];
            post_state = (post_state >> 8) & 0x0000FFFF;
            if(test_bit(post_state, desc->deny))
            {
                return RT_ER;
            }
            else
            {
                head++;
            }
        }

        for(i = 0; i < MAX_BITSET; ++i) {
            if(desc->dely[i] & run_set[i]) {
                return RT_DE;
            }
        }

        head = qhead;
        tail = qtail;
        while(head < tail) 
        {
            int idx = head % MAX_MESSAGE;
            int post_state = msg_queue[idx][0];
            post_state = (post_state >> 8) & 0x0000FFFF;
            if(test_bit(post_state, desc->dely))
            {
                return RT_DE;
            }
            else
            {
                head++;
            }
        }
        return RT_OK;
    }
    return RT_ER;
}

bool State::can_be_state_late( int index )
{
    if (index >= 0 && index < STATE_MAX)
    {
        StateDesc *desc = &v_state[index];
        unsigned int i;
        for(i = 0; i < MAX_BITSET; ++i)
        {
            if(desc->deny[i] & run_set[i])
            {
                return false;
            }
        }

        int head = qhead;
        int tail = qtail;
        while(head < tail) 
        {
            int idx = head % MAX_MESSAGE;
            int post_state = msg_queue[idx][0];
            post_state = (post_state >> 8) & 0x0000FFFF;
            if(test_bit(post_state, desc->deny))
            {
                return false;
            }
            else
            {
                head++;
            }
        }
        return true;
    }
    return false;
}

int State::clear_msg_queue()
{
    qtail = qhead = 0;  
    return RT_OK;
}

void State::init()
{
}

void* State::get_obj()
{
    return obj;
}

void State::set_obj( void *_obj )
{
    obj = _obj;
}

void State::set_param( int index, int idx, int value )
{
    if(index >= 0 && index < STATE_MAX) {
        if(idx >= 0 && idx < MAX_PARAM) {
            data[index][idx] = value;
        }
    }
}

int State::get_param( int index, int idx )
{
    if(index >= 0 && index < STATE_MAX) {
        if(idx >= 0 && idx < MAX_PARAM) {
            return data[index][idx];
        }
    }
    return 0;
}

void State::set_all( int index, int p0, int p1, int p2, int p3 )
{
    if(index > 0 && index < STATE_MAX) {
        data[index][0] = p0;
        data[index][1] = p1;
        data[index][2] = p2;
        data[index][3] = p3;
    }
}

int State::is_top( int index )
{
    return ((int)top == index);
}

bool State::is_state_in_post( int index )
{
    int head = qhead;
    int tail = qtail;

    while(head < tail) 
    {
        int idx = head % MAX_MESSAGE;
        int post_state = msg_queue[idx][0];
        post_state = (post_state >> 8) & 0x0000FFFF;
        if( post_state == index )
        {
            return true;
        }
        else
        {
            head++;
        }
    }
    return false;
}

enum {
    T_INT,
    T_COMMA,
    T_SEMI,
    T_DOLL,
    T_END,
    T_ERROR
};


//static int get_token( char **p, char *e, int *val )
//{
//    if(*p >= e) return T_END;
//
//    char node[32] = {0,};
//    int index = 0;
//    while(*p < e) {
//        if((**p >= '0' && **p <= '9') || (**p) == '-') {
//            node[index++] = **p;
//            (*p)++;
//        } else {
//            break;
//        }     
//    }
//
//    if(index > 0) {
//        *val = atoi(node);
//        return T_INT;
//    }
//    
//    switch(**p) {
//        case ',':
//            (*p)++;
//            return T_COMMA;
//        case ';' :
//            (*p)++;
//            return T_SEMI;
//        case '$':
//            (*p)++;
//            return T_DOLL;
//    }
//
//    return T_ERROR;
//}
//
/*
void State::serialize( Ar& ar)
{
    int i = 0;
    if(ar.is_storing() ) {
        char str_state[1024] = {0,};
        char node[124];

        //state and data
        int index = -1;
        while((index = find_next_set_bit(run_set, (sizeof(run_set) << 3), index + 1)) < (int)(sizeof(run_set) << 3)) {
            for(i = 0; i < MAX_PARAM; ++i) {
                if(data[index][i]) {
                    int key = index * 10 + i;
                    sprintf(node, "%d,%d;",key, data[index][i]);
                    strcat(str_state, node);
                }
            }
        }
        strcat(str_state, "$");

        //msg_queue
        int t_head = qhead;
        while(t_head < qtail) {
            index = t_head % MAX_MESSAGE;
            sprintf(node, "%d,%d,%d,%d,%d;", msg_queue[index][0], msg_queue[index][1], msg_queue[index][2], msg_queue[index][3], msg_queue[index][4]);
            strcat(str_state, node);
            t_head++;
        }

        strcat(str_state, "$");
        ar.write_string(str_state, strlen(str_state));

    } else {
        char str_state[1024] = {0,};
        ar.read_string(str_state, sizeof(str_state));     

        char *p, *end;
        p = str_state;
        end = p + strlen(str_state);

        int tok;
        int num;
        int tmp;
        bool ok;

        int index;
        tok = get_token(&p, end, &num);
        while(tok != T_END && tok != T_DOLL) {
            if(tok == T_INT) {
                ok = false;
                index = num;
                if( T_COMMA == get_token(&p, end, &tmp)) {
                    if(T_INT == get_token(&p, end, &num)) {
                        if(T_SEMI == get_token(&p, end, &tmp)) {
                            set_bit(index / 10, run_set);  
                            set_param(index / 10, index % 10, num); 
                            ok = true;
                        }
                    }
                }
                if(!ok) return;
            }
            tok = get_token(&p, end, &num);
        }

        int msg, p0, p1, p2, p3;
        tok = get_token(&p, end, &num);
        while(tok != T_END && tok != T_DOLL) {
            if(tok == T_INT) {
                msg = num;
                get_token(&p, end, &tmp);
                get_token(&p, end, &p0);
                get_token(&p, end, &tmp);
                get_token(&p, end, &p1);
                get_token(&p, end, &tmp);
                get_token(&p, end, &p2);
                get_token(&p, end, &tmp);
                get_token(&p, end, &p3);
                tok = get_token(&p, end, &tmp);
                if(tok != T_SEMI) return;

                msg_queue[qtail][0] = msg;
                msg_queue[qtail][1] = p0;
                msg_queue[qtail][2] = p1;
                msg_queue[qtail][3] = p2;
                msg_queue[qtail][4] = p3;
                qtail++;

            }
            tok = get_token(&p, end, &num);
        }
    }
}
*/

/**
 *   +-----------------------+
 *   |state_num:char         |
 *   |+-------------------+* |
 *   ||state:char         |  |
 *   ||param_num:char     |  |
 *   ||+-------+--------+*|  |
 *   |||param_id:char   | |  |
 *   |||param_value:int | |  |
 *   ||+----------------+ |  |
 *   |+-------------------+  |
 *   |msg_queue_num:char     |
 *   |+-------------------+* |
 *   ||msg:int            |  |
 *   ||param0:int         |  |
 *   ||param1:int         |  |
 *   ||param2:int         |  |
 *   ||param3:int         |  |
 *   |+-------------------+  |
 *   +-----------------------+
 **/
void State::serialize( Ar& ar)
{
    if(ar.is_storing() ) 
    {
        // write state num
        unsigned char num = 0;
        int offset = ar.get_offset();
        ar << num;
        // write state data
        unsigned char count = 0;
        int index = -1;
        while((index = find_next_set_bit(run_set, (sizeof(run_set) << 3), index + 1)) < (int)(sizeof(run_set) << 3)) {
            unsigned char count2 = 0;
            count++;
            ar << (unsigned char)index;
            int offset2 = ar.get_offset();
            unsigned char num2 = 0;
            ar << num2;
            for(int i = 0; i < MAX_PARAM; ++i) {
                if(data[index][i]) {
                    count2++;
                    ar << (unsigned char)i;
                    ar << data[index][i];
                }
            }
            ar.write_byte_at( count2, offset2 );
        }
        ar.write_byte_at( count, offset );
    }
    else 
    {
        // read state
        unsigned char num = 0;
        ar >> num;
        while( num-- > 0 )
        {
            unsigned char index;
            ar >> index;
            unsigned char data_num;
            ar >> data_num;
            while( data_num-- > 0 )
            {
                unsigned char id;
                int data;
                ar >> id >> data;
                set_param(index, id, data); 
            }
            send_msg( MK_MSG(SYS_SET_STATE, index), get_param(index, 0), get_param(index, 1), get_param(index, 2), get_param(index, 3) );
        }
    }
}

int State::set_state_func( int state, FUNC_HANDLE handle )
{
    if(state >= 0 && state < STATE_MAX) {
        v_state[state].handle = handle;
        return RT_OK;
    } 
    return RT_ER;
}

StateDesc* State::get_desc( int index )
{
    return &v_state[index]; 
}


