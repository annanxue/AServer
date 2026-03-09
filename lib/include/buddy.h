#ifndef _BUDDY_H
#define _BUDDY_H

#include "mylist.h"
#include "autolock.h"

typedef struct{
    list_head       link;
    unsigned int    order;
    unsigned int    param;/*!< now it just signed for use, it can take more info just you want */
}page_t;

typedef struct{
    list_head       free_list;
    unsigned int    nr_free; 
} free_area_t;

class Buddy
{
    public:
        ~Buddy();
        int     init(unsigned int size_min, unsigned int size_max, unsigned int max_block_count);

        void*   get(unsigned long order);
        bool    put(void* p );

        void    print();
        
    private:
        free_area_t free_area[ 32 ];
        page_t*         pages;
        void*           start;
        void*           end;
        Mutex           lock;

        unsigned int    free_pages;
        unsigned int    page_size; 
        unsigned int    page_shift;
        unsigned int    page_count;
        unsigned int    max_order;
};


typedef struct{
    list_head       link;
    char            data[];
}page2_t;

typedef struct{
    list_head       free_list;
    unsigned        nr_free;
    unsigned char   *map;
} free_area2_t;

class MiniBuddy
{
    public:

        ~MiniBuddy();
        int     init(unsigned int size_min, unsigned int size_max, unsigned int max_block_count);
        bool    put_size(void *p, unsigned int size );
        void*   get_size( unsigned int size );
        void*   reget_size( void* p, unsigned int oldsize, unsigned int newsize );

        void    print();
        
        void*   get(unsigned int order);
        bool    put(void* p, unsigned int order );

        unsigned int num_new(){ return more;}
        unsigned int num_page(){ return free_pages;}
        unsigned int num_use() { return page_count - free_pages; }

    private:
        free_area2_t    free_area[ 32 ];

        unsigned int    start;
        unsigned int    end;

        unsigned int    free_pages;
        unsigned int    more;
        unsigned int    page_shift;
        unsigned int    page_count;
        unsigned int    max_order;

        Mutex            lock;
};

#endif 


