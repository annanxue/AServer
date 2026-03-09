#ifdef __linux

#ifndef _FFCTX_H_
#define _FFCTX_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned long sd;
	int dis1;
	int dis2;
	int index;
	int carry;  
	unsigned long buffer[64];
} ff_addikey;


typedef struct {
	unsigned long sd;
	int index;
	ff_addikey addikey[3];
	unsigned char buffer[0x1000];
} ff_ctx;


void ctx_init(unsigned long sd, ff_ctx &ctx);
void ctx_encode(ff_ctx &ctx, void *buffer, int len);
void apply_signature(int signature);

#ifdef __cplusplus                                                            
}                                                                             
#endif    

#endif

#endif //__linux
