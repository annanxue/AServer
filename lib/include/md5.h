#ifndef _ARITHMD5_H__
#define _ARITHMD5_H__

struct MD5_CTX
{
	unsigned long	state[4];				/* state (ABCD) */
	unsigned long	count[2];				/* number of bits, modulo 2^64 (lsb first) */
	unsigned char	buffer[64];				/* input buffer */
};

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned long);
void MD5Final(unsigned char[16], MD5_CTX *);
void MD5(unsigned char *szSour, int iLen, unsigned char *szDest);

#endif /*_ARITHMD5_H__*/
