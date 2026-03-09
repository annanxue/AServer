#ifndef _MIN_HEAP_H_
#define _MIN_HEAP_H_

#include <sys/time.h>
#include <stdlib.h>
using namespace std;

struct min_heap_item_t
{
    int min_heap_idx;
    int id;
    char ip[16];
    int port;
    int user_num;
};

typedef struct min_heap
{
    struct min_heap_item_t** p;
    unsigned n, a;
}min_heap_t;

static inline void min_heap_ctor(min_heap_t* s);
static inline void min_heap_dtor(min_heap_t* s);
static inline void min_heap_elem_init(struct min_heap_item_t* e);
static inline int min_heap_elem_greater(struct min_heap_item_t* a, struct min_heap_item_t* b);
static inline int min_heap_empty(min_heap_t* s);
static inline unsigned min_heap_size(min_heap_t* s);
static inline struct min_heap_item_t* min_heap_top(min_heap_t* s);
static inline int min_heap_reserve(min_heap_t* s, unsigned n);
static inline int min_heap_push(min_heap_t* s, struct min_heap_item_t* e);
static inline struct min_heap_item_t* min_heap_pop(min_heap_t* s);
static inline int min_heap_erase(min_heap_t* s, struct min_heap_item_t* e);
static inline void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct min_heap_item_t* e);
static inline void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct min_heap_item_t* e);

int min_heap_elem_greater(struct min_heap_item_t* a, struct min_heap_item_t* b)
{
    return a->user_num > b->user_num;
}

void min_heap_ctor(min_heap_t* s)
{
    s->p = 0;
    s->n = 0;
    s->a = 0;
}

void min_heap_dtor(min_heap_t* s)
{
    if(s->p)
    {
        free(s->p);
    }
}

void min_heap_elem_init(struct min_heap_item_t* e)
{
    e->min_heap_idx = -1;
}

int min_heap_empty(min_heap_t* s)
{
    return 0 == s->n;
}

unsigned min_heap_size(min_heap_t* s)
{
    return s->n;
}

struct min_heap_item_t* min_heap_top(min_heap_t* s)
{
    return s->n ? *s->p : 0;
}

int min_heap_push(min_heap_t* s, struct min_heap_item_t* e)
{
    if(min_heap_reserve(s, s->n + 1))
    {
        return -1;
    }
    min_heap_shift_up_(s, s->n++, e);
    return 0;
}

struct min_heap_item_t* min_heap_pop(min_heap_t* s)
{
    if(s->n)
    {
        struct min_heap_item_t* e = *s->p;
        min_heap_shift_down_(s, 0, s->p[--s->n]);
        e->min_heap_idx = -1;
        return e;
    }
    return 0;
}

int min_heap_erase(min_heap_t* s, struct min_heap_item_t* e)
{
    if(-1 != e->min_heap_idx)
    {
        struct min_heap_item_t* last = s->p[--s->n];
        unsigned parent = (e->min_heap_idx - 1) / 2;
        if(e->min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
        {
            min_heap_shift_up_(s, e->min_heap_idx, last);
        }
        else
        {
            min_heap_shift_down_(s, e->min_heap_idx, last);
        }
        e->min_heap_idx = -1;
        return 0;
    }
    return -1;
}

int min_heap_reserve(min_heap_t* s, unsigned n)
{
    if(s->a < n)
    {
        struct min_heap_item_t** p;
        unsigned a = s->a ? s->a * 2 : 8;
        if(a < n)
        {
            a = n;
        }
        if(!(p = (struct min_heap_item_t**)realloc(s->p, a*sizeof*p)))
        {
            return -1;
        }
        s->p = p;
        s->a = a;
    }
    return 0;
}

void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct min_heap_item_t* e)
{
    unsigned parent = (hole_index - 1) / 2;
    while(hole_index && min_heap_elem_greater(s->p[parent], e))
    {
        (s->p[hole_index] = s->p[parent])->min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->min_heap_idx = hole_index;
}

void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct min_heap_item_t* e)
{
    unsigned min_child = 2 * (hole_index + 1);
    while(min_child <= s->n)
    {
        min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
        if(!(min_heap_elem_greater(e, s->p[min_child])))
        {
            break;
        }
        (s->p[hole_index] = s->p[min_child])->min_heap_idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
    }
    (s->p[hole_index] = e)->min_heap_idx = hole_index;
}

#endif /*_MIN_HEAP_H_*/
