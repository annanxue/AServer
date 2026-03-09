#ifndef __MALLOC_HOOK__
#define __MALLOC_HOOK__

#ifdef __cplusplus
extern "C" {
#endif 


void  ff_init_malloc_hook(void);
void  ff_reset_malloc_hook(void);
void* ff_malloc_hook(size_t,const void*);
void  ff_free_hook(void*,const void *);

void  print_backtrace(void);

#ifdef __cplusplus
}
#endif

#endif
