
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "buddy.h"
#include <assert.h>


void* Buddy::get( unsigned long order )
{
    AutoLock auto_lock( &lock );
    free_area_t* area;
    unsigned int current_order;
    page_t *page, *target;
    for( current_order = order; current_order <= max_order; ++current_order ) {
        area = free_area + current_order;
        if( list_empty(&area->free_list) ) continue;
        page = list_entry( area->free_list.next, page_t, link ); 
        list_del(&page->link); 
        area->nr_free--;
        target = page;

        int high = current_order;
        int low = order;
        unsigned long size = 1 << high;
        while(high > low) {
            area--;
            high--;
            size >>= 1;
            area->nr_free++;
            list_add(&page[size].link, &area->free_list);
            page[size].order = high;
            page[size].param = 0;
        }

        target->order = order;
        target->param = 1;
        free_pages -= 1UL << order;
        return (void*)( ((target-pages) << page_shift) + (unsigned int)start );
    }

    void* p = malloc( 1 << (page_shift + order ) ); 

    return p;
}

bool Buddy::put( void* target )
{
    AutoLock auto_lock( &lock );
    if( target < start || target >= end ) {
        free( target );
        return true;
    }

    unsigned int offset = (unsigned int)target - (unsigned int)start;
    unsigned int index = offset >> page_shift;
    if( index < page_count ) {
        page_t* page = &pages[index];
        if( page->param ) {
            unsigned int order = page->order;
            if( ( offset & ( ~( ~0UL << ( page_shift + order ) ) ) ) == 0 ) {
                free_pages += 1 << order;
                page->param = 0; 
                while(order < max_order) {
                    page_t* buddy = &pages[ index ^ (1 << order) ];
                    if( buddy->param == 0 && buddy->order == order ) {
                        list_del( &buddy->link );
                        free_area_t* area = free_area + order;
                        area->nr_free--;
                        index = index & ~(1 << order );
                        order++;
                    } else {
                        break;
                    }
                }
                list_add( &pages[index].link, &free_area[order].free_list);
                pages[index].order = order;
                free_area[order].nr_free++;
                return true;
            }
        }
    }
    return false;
}

int Buddy::init( unsigned int size_min, unsigned int size_max, unsigned int max_block_count )
{
    page_shift = 0;
    max_order = 0;
    unsigned int i;
    for(i = 0; i < 32; ++i ) {
        if( size_min == 1UL << i ) page_shift = i;
        if( size_max == 1UL << i ) max_order = i - page_shift;
        INIT_LIST_HEAD( &free_area[i].free_list );
        free_area[i].nr_free = 0;
    }

    if( page_shift == 0 || max_order == 0 ) return -1;

    page_size = 1 << page_shift;
    page_count = (1 << max_order) * max_block_count;

    unsigned int max_block_size = 1 << (max_order + page_shift);
    start = malloc(max_block_size * max_block_count);
    pages = (page_t*)malloc(page_count * sizeof(page_t) );
    memset(pages, 0, page_count * sizeof(page_t));
    memset( pages, 0, page_count * sizeof(page_t) );
    memset( start, 0, max_block_size * max_block_count);
    end = (void*)( (unsigned int)start + max_block_size * max_block_count );

    unsigned int offset = 1 << max_order;
    for(i = 0; i < max_block_count; ++i) {
        page_t* page = &pages[offset * i];
        page->order = max_order;
        page->param = 0;
        list_add_tail( &page->link, &free_area[max_order].free_list );
    }
    free_area[max_order].nr_free = max_block_count;
    free_pages = (1 << max_order) * max_block_count;
    print();
    return 0;
}

void Buddy::print()
{
}

Buddy::~Buddy()
{
    if (start) {
        free((void*)start);
        start = 0;
    }
    if (pages) {
        free((void*)pages);
        pages = 0;
    }
}

//MiniBuddy -----------
int MiniBuddy::init( unsigned int size_min, unsigned int size_max, unsigned int max_block_count )
{
    more = 0;
    page_shift = 0;
    max_order = 0;
    unsigned int i;
    for(i = 0; i < 32; ++i ) {
        if( size_min == 1UL << i ) page_shift = i;
        if( size_max == 1UL << i ) max_order = i - page_shift;
        INIT_LIST_HEAD( &free_area[i].free_list );
        free_area[i].nr_free = 0;
        free_area[i].map = 0;
    }

    if( page_shift == 0 || max_order == 0 ) return -1;

    page_count = (1 << max_order) * max_block_count;

    unsigned int max_block_size = 1 << (max_order + page_shift);
    start = (unsigned int)malloc(max_block_size * max_block_count);
    end = start + max_block_count * max_block_size;
    memset( (void*)start, 0, max_block_size * max_block_count);

    unsigned int offset = 1 << (max_order + page_shift);
    for(i = 0; i < max_block_count; ++i) {
        page2_t* page = (page2_t*)(start + i * offset);
        list_add_tail( &page->link, &free_area[max_order].free_list );
    }

    free_area[max_order].nr_free = max_block_count;
    free_pages = (1 << max_order) * max_block_count;

    for(i = 0; i <= max_order; ++i ) {
        //unsigned int bitmap_size = (page_count-1) >> (i+4); /*! byte */
        //bitmap_size = ( ( bitmap_size ) + ( sizeof(unsigned int) ) - 1 ) &~ ( ( sizeof(unsigned int) ) - 1 );

        unsigned int bitmap_size = (page_count) >> (i+4);
        bitmap_size += 1;

        free_area[i].map = (unsigned char*)malloc( bitmap_size );
        memset( free_area[i].map, 0, bitmap_size );
    }
    //print();
    return 1;
}

static inline int test_and_change_bit(int nr, volatile void *addr)
{
    unsigned int mask = 1 << (nr & 7);
    unsigned int oldval;

    oldval = ((unsigned char *) addr)[nr >> 3];
    ((unsigned char *) addr)[nr >> 3] = oldval ^ mask;
    return oldval & mask;
}

bool MiniBuddy::put( void* p, unsigned int order )
{
    //AutoLock lock( &lock );
    if( (unsigned int)p < start || (unsigned int)p >= end ) {
        free( p );
        return true;
    }

    unsigned int offset = (unsigned int)p - start;
    if( offset & ((1 << page_shift) - 1) ) {
        return false;
    }

    unsigned int mask = (~0UL) << order;
    unsigned int page_idx = offset >> page_shift; 
    if( page_idx & ~mask ) {
        return false;
    }

    unsigned int index = page_idx >> ( 1 + order );
    free_pages += (1 << order);

    free_area2_t* area = free_area + order; 
    while( order < max_order ) {
        if( !test_and_change_bit( index, area->map ) ) break;
        page2_t* buddy = (page2_t*)(start + ((page_idx ^ -mask) << page_shift));
        list_del( &buddy->link );
        mask <<= 1;
        index >>= 1;
        page_idx &= mask;
        area++;
        order++;
    }

    page2_t* page = (page2_t*)(start + ((page_idx) << page_shift));
    list_add( &page->link, &area->free_list ); 

    //print();
    return false;
}

void* MiniBuddy::get( unsigned int order )
{
    //AutoLock lock( &lock );
    free_area2_t* area = free_area + order;
    unsigned int curr_order = order;
    list_head *head, *curr;
    page2_t *page;

    while(curr_order <= max_order) {
        head = &area->free_list;
        curr = head->next;
        if(curr != head) {
            page = list_entry(curr, page2_t, link);
            list_del(curr);
            unsigned int index = ((unsigned int)page - start) >> page_shift;
            test_and_change_bit( index >> (1 + curr_order), area->map);

            free_pages -= 1UL << order;
            if( curr_order > order ) {
                unsigned int high = curr_order;
                unsigned int low = order;
                unsigned int size = 1 << high;
                while (high > low) {
                    area--;
                    high--;
                    size >>= 1;
                    list_add(&(page)->link, &(area)->free_list);
                    test_and_change_bit(index >> (1 + high), area->map);
                    index += size;
                    page = (page2_t*)(start + (index << page_shift));
                }
            }

            return (void*)page;
        }
        curr_order++;
        area++;
    }
    return NULL; 
}

void* MiniBuddy::get_size( unsigned int size )
{
    void* p = NULL;
    unsigned int order;
    for( order = 0; order <= 32; ++order ) {
        if(size <= (unsigned int)(1 << (order + page_shift))) {
            p = get(order);  
            break;
        }
    }

    if( !p ) {
        p = realloc( NULL, size );
        //AutoLock lock( &lock );
        more += size;
    }

    return p;
}

bool MiniBuddy::put_size( void* p, unsigned int size )
{
    if( size == 0) return true;

    if( (unsigned int)p < start || (unsigned int)p >= end ) {
        free(p);
        return true;
    }

    unsigned int order;
    for(order = 0; order <= 32; ++order) {
        if(size <= (unsigned int)(1 << (order + page_shift))) {
            return put(p, order);
        }
    }

    return false;
}

void* MiniBuddy::reget_size( void* p, unsigned int osize, unsigned int nsize )
{
    if(p && ((unsigned int)p < start || (unsigned int)p >= end)){
        p = realloc(p, nsize);
        //AutoLock lock( &lock );
        more += (nsize - osize);
        return p;
    }

    if(p) {
        if(nsize == 0) {    // free 
            put_size(p, osize); 
            return NULL;
        } else {            // realloc 
            unsigned int order; 
            unsigned int oorder = 1000; 
            unsigned int norder = 1000;

            for(order = 0; order <= 32; ++order) {
                if((unsigned int)(1 << (order + page_shift)) >= osize) {
                    if(oorder == 1000) oorder = order;
                }

                if((unsigned int)(1 << (order + page_shift)) >= nsize) {
                    if(norder == 1000) norder = order;
                }
                if(oorder <= max_order && norder <= max_order) break;
            }

            if( oorder == norder ) return p;
            void* np = get(norder);
            if( !np ) { 
                np = realloc( NULL, nsize );
                //AutoLock lock( &lock );
                more += (nsize - osize);
            }

            if( np ) {
                memcpy( np, p, osize < nsize ? osize : nsize );
                put_size( p, osize );
            }
            return np;
        } 
    } else {
        if(nsize) {        // new
            p = get_size( nsize );
            if(!p) {
                p = malloc( nsize );
                //AutoLock lock( &lock );
                more += (nsize - osize);
            }

            return p;
        } 
    }
    return NULL;
}

void MiniBuddy::print()
{
}


MiniBuddy::~MiniBuddy()
{
    if (start) {
        free((void*)start);
        start = 0;
    }   

    int i = 0;
    for (i = 0; i < 32; ++i) {
        if (free_area[i].map) {
            free((void*)free_area[i].map);
            free_area[i].map  = 0;
        }   
    }   
}

