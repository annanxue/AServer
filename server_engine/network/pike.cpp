#ifdef __linux

#include "pike.h"

const int DEFAULT_GENIUS_NUMBER = 0x05027919;
int GENIUS_NUMBER = DEFAULT_GENIUS_NUMBER;

void apply_signature(int signature)
{
    if (GENIUS_NUMBER == DEFAULT_GENIUS_NUMBER)
    {
        GENIUS_NUMBER ^= signature;
    }
}

/*! 参见<<应用密码学>>中的线性反馈移位寄存器算法*/
static void linearity(unsigned long &key)
{
	key =  (((( key >> 31 )
			^ ( key >> 6  )
			^ ( key >> 4  )
			^ ( key >> 2  )
			^ ( key >> 1  )
			^   key       )
			&   0x00000001 )
			<<  31         )
			| ( key >> 1   );
}


static void addikey_next(ff_addikey &addikey)
{
	(++addikey.index) &= 0x03F;

	int i1 = ((addikey.index | 0x40) - addikey.dis1) & 0x03F;
	int i2 = ((addikey.index | 0x40) - addikey.dis2) & 0x03F;

	addikey.buffer[addikey.index] =  addikey.buffer[i1] + addikey.buffer[i2]; //  % 0xFFFFFFFF;   /*!< 让它自动溢出,如果是64位机器,则要避免取模*/
	addikey.carry = (addikey.buffer[addikey.index] < addikey.buffer[i1]) || (addikey.buffer[addikey.index] < addikey.buffer[i2]) ;   /*!< 如果小于任意一个则表示有进位	*/
}


static void ctx_generate(ff_ctx &ctx)
{
	for(int i = 0; i < 1024; ++i)
	{
		int carry = ctx.addikey[0].carry + ctx.addikey[1].carry + ctx.addikey[2].carry;

		if (carry ==0 || carry == 3)   /*!< 如果三个位相同(全0或全1),那么钟控所有的发生器*/
		{
			addikey_next(ctx.addikey[0]);
			addikey_next(ctx.addikey[1]);
			addikey_next(ctx.addikey[2]);
		}
		else /*!< 如果三个位不全相同,则钟控两个相同的发生器*/
		{
			int flag = 0;
			if(carry == 2) flag = 1;
			for(int j = 0; j < 3; ++j) 
			{
				if(ctx.addikey[j].carry == flag)  addikey_next(ctx.addikey[j]);
			}
		}

		*(unsigned long *)(&ctx.buffer[i*4]) =   ctx.addikey[0].buffer[ctx.addikey[0].index]
											   ^ ctx.addikey[1].buffer[ctx.addikey[1].index]
											   ^ ctx.addikey[2].buffer[ctx.addikey[2].index];
	}
	
	ctx.index = 0;
}


void ctx_init(unsigned long sd, ff_ctx &ctx)
{
	unsigned long tmp;

	ctx.sd = sd ^ GENIUS_NUMBER;
	
	/*! 参见<<应用密码学>>中的Pike算法*/
	ctx.addikey[0].sd = ctx.sd;
	linearity(ctx.addikey[0].sd);
	ctx.addikey[0].dis1 = 55;
	ctx.addikey[0].dis2 = 24;


	ctx.addikey[1].sd =   ((ctx.sd & 0xAAAAAAAA) >> 1)|( (ctx.sd & 0x55555555) <<1 );
	linearity(ctx.addikey[1].sd);
	ctx.addikey[1].dis1 = 57;
	ctx.addikey[1].dis2 = 7;

	
	ctx.addikey[2].sd =  ~(((ctx.sd & 0xF0F0F0F0) >> 4)|( (ctx.sd & 0x0F0F0F0F) <<4 ));
	linearity(ctx.addikey[2].sd);
	ctx.addikey[2].dis1 = 58;
	ctx.addikey[2].dis2 = 19;


	for(int i = 0; i < 3; ++i)
	{
		tmp = ctx.addikey[i].sd;
		for(int j = 0; j < 64; ++j)
		{
			for(int k = 0; k < 32; ++k)
			{
				linearity(tmp);
			}
			ctx.addikey[i].buffer[j] = tmp;
		}
		ctx.addikey[i].carry = 0;	/*!< 进位标志初始为0*/
		ctx.addikey[i].index = 63;  /*!< 当前位为第63位*/
	}

	ctx.index = 4096;

	//ctx_generate(ctx);
}


void ctx_encode(ff_ctx &ctx, void *buffer, int len)
{
	if(len <= 0)  return;
	if(!buffer) return; /*!<其实意义不大*/

	unsigned char *data=(unsigned char *)buffer;
	unsigned char *ptr = 0;

	do {

		int remnant = 4096 - ctx.index;
		if (remnant <= 0)    /*!< 序列用完,重新生成*/
		{
			ctx_generate(ctx);
			continue;
		}
		

		if (remnant > len) remnant = len;
		len -= remnant;

		ptr = ctx.buffer + ctx.index;
		
		int i = 0;
		for(; i < remnant - 3; i+=4, data+=4, ptr+=4)
		{
			*(unsigned long *)data ^= *(unsigned long *)ptr;			
		}
		for(; i < remnant; ++i, ++data, ++ptr)
		{
			*data ^= *ptr;
		}
		
		ctx.index += remnant;		
	} while (len>0);		
}

#endif //__linux
