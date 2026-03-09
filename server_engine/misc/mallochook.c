#include "stdio.h"
#include <malloc.h>
#include "mallochook.h"

extern FILE* fmem_check;
#define FMEMLOG fmem_check

/*************************************************************
    malloc hook 
 *************************************************************/

static int ff_malloc_hook_flag = 0;

static void* (* old_malloc_hook) (size_t,const void *);
static void (* old_free_hook)(void *,const void *);

void ff_init_malloc_hook(void)
{
    if( ff_malloc_hook_flag == 1 )
    {
        fprintf( FMEMLOG, "[MEM_HOOK] re init malloc hook ...\n" );
        return;
    }

    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    __malloc_hook = ff_malloc_hook;
    __free_hook = ff_free_hook;

    ff_malloc_hook_flag = 1;
}

void ff_reset_malloc_hook(void)
{
    if( ff_malloc_hook_flag == 0 )
    {
        fprintf( FMEMLOG, "[MEM_HOOK] re reset malloc hook ...\n" );
        return;
    }

    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    
    old_malloc_hook = NULL;
    old_free_hook = NULL;
    
    ff_malloc_hook_flag = 0;

    if( FMEMLOG )
    {
        fprintf( FMEMLOG, "[MEM_HOOK] finish ...\n" );
    }
}

void* ff_malloc_hook(size_t size,const void *caller)
{
    void *result;
    __malloc_hook = old_malloc_hook;
    result = malloc(size);
    old_malloc_hook = __malloc_hook;
    if( FMEMLOG )
    {
        fprintf( FMEMLOG, "[MEM_HOOK]@@@ %p ++ %p Size = %u\n", caller, result, (unsigned int)size );
    }
    __malloc_hook = ff_malloc_hook;

    return result;
}

void ff_free_hook(void *ptr,const void *caller)
{
    __free_hook = old_free_hook;
    free(ptr);
    old_free_hook = __free_hook;
    if( FMEMLOG )
    {
        fprintf( FMEMLOG, "[MEM_HOOK]@@@ %p -- %p\n", caller, ptr );
    }
    __free_hook = ff_free_hook;
}

