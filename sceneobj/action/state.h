#ifndef _STATE_H_
#define _STATE_H_

#include "states.h"
#include "ar.h"

#define MAX_PARAM   12
#define MAX_MESSAGE 16

#define MK_MSG( id, state ) (id + (state << 8))

class State;
typedef int (*FUNC_HANDLE)( State *state, int index, int msg, int p0, int p1, int p2, int p3 );
typedef struct{
    unsigned int deny[MAX_BITSET];
    unsigned int swap[MAX_BITSET];
    unsigned int dely[MAX_BITSET];
    FUNC_HANDLE handle;
} StateDesc;


inline void set_bit(unsigned int nr,unsigned int *addr)
{
    int mask;
    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    *addr |= mask;
}

inline void clear_bit(unsigned int nr, unsigned int *addr)
{
    int mask;
    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    *addr &= ~mask;
}

inline unsigned int test_bit(unsigned int nr, unsigned int *addr)
{
    int mask;
    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    return ((mask & *addr) != 0);
}

class State
{
    public:
        State();
        ~State();

    public:
        int del_state( unsigned int index );
        int del_state_from_msg_queue( unsigned int index );
        int is_state( unsigned int index );

        void process();
        /*! -1 error, 0 not handle, > 0 handle */
        int send_msg( int msg, int p0, int p1, int p2, int p3 );
        int post_msg( int msg, int p0, int p1, int p2, int p3 );
        
        void* get_obj();
        void set_obj( void *obj );        

        void set_param( int index, int idx, int value ); 
        int get_param( int index, int idx );
        void set_all( int index, int p0, int p1, int p2, int p3 );

        int is_top( int index );

	    void serialize( Ar& ar);
        int do_send_msg( int msg, int p0, int p1, int p2, int p3 );

        void pause( unsigned int index );
        int can_be_state( unsigned int index );
        bool can_be_state_late( int index );
        int clear_msg_queue();
        bool is_state_in_post( int index );

        unsigned int run_set[MAX_BITSET];
        int msg_queue[MAX_MESSAGE][5];
        int qhead;
        int qtail;
    private:
        unsigned int pause_set[MAX_BITSET];

        int data[STATE_MAX][MAX_PARAM];

        void *obj;
        unsigned int top;

        int msg_tm;

    public:
        static void init();
        static StateDesc* get_desc( int index );
        static int set_state_func( int index, FUNC_HANDLE handle );

    private:

        /*! index 0 for v_state and group should be no use */
        static StateDesc    v_state[ STATE_MAX ];
    public:
        static void set_msg_retry_max(int max) { msg_retry_max = max; }
        static int msg_retry_max;
};

#define RT_NO           0
#define RT_ER           -1
#define RT_DE           -2
#define RT_OK           1


#define SYS_SET_STATE   0
#define SYS_DEL_STATE   1
#define MSG_INITIAL     3
#define MSG_DESTROY     4
#define MSG_GO_TOP      5
#define MSG_UPDATE      6

#define MSG_SETDATA     7
#define MSG_RESET       8
#define MSG_GETDATA     9 

#define FLOAT2INT_MUL_FACTOR 1000
#define INT2FLOAT_MUL_FACTOR 0.001f


#endif



