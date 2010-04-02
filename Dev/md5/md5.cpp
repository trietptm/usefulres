#include <Windows.h>
#include <stdio.h>
#include "md5.h"

static unsigned char PADDING[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* The routine MD5Init initializes the message-digest context
	mdContext. All fields are set to zero.
*/
void MD5Init(MD5_CTX *mdContext)
{
	mdContext->i[0] = mdContext->i[1] = (UINT4)0;

	/* Load magic initialization constants.
	*/
	mdContext->buf[0] = (UINT4)0x67452301;
	mdContext->buf[1] = (UINT4)0xefcdab89;
	mdContext->buf[2] = (UINT4)0x98badcfe;
	mdContext->buf[3] = (UINT4)0x10325476;
}


/* The routine MD5Update updates the message-digest context to
account for the presence of each of the characters inBuf[0..inLen-1]
in the message whose digest is being computed.
*/
void MD5Update(MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;

	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* update number of bits */
	if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
		mdContext->i[1]++;
	mdContext->i[0] += ((UINT4)inLen << 3);
	mdContext->i[1] += ((UINT4)inLen >> 29);

	/* Speedup for little-endian machines suggested in MD5 report --P Karn */
	if(mdi == 0 && ((int)inBuf & 3) == 0){
		while(inLen >= 64){
			MD5Transform(mdContext->buf,(UINT4 *)inBuf);
			inLen -= 64;
			inBuf += 64;
		}               
	}

	while (inLen--) {
		/* add new character to buffer, increment mdi */
		mdContext->in[mdi++] = *inBuf++;

		/* transform if necessary */
		if (mdi == 0x40) {
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
				(((UINT4)mdContext->in[ii+2]) << 16) |
				(((UINT4)mdContext->in[ii+1]) << 8) |
				((UINT4)mdContext->in[ii]);
			MD5Transform (mdContext->buf, in);
			mdi = 0;
		}
	}
}

/* The routine MD5Final terminates the message-digest computation and
ends with the desired message digest in mdContext->digest[0...15].
*/
void MD5Final(MD5_CTX *mdContext)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;
	unsigned int padLen;

	/* save number of bits */
	in[14] = mdContext->i[0];
	in[15] = mdContext->i[1];

	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* pad out to 56 mod 64 */
	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	MD5Update (mdContext, PADDING, padLen);

	/* append length in bits and transform */
	for (i = 0, ii = 0; i < 14; i++, ii += 4)
		in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
		(((UINT4)mdContext->in[ii+2]) << 16) |
		(((UINT4)mdContext->in[ii+1]) << 8) |
		((UINT4)mdContext->in[ii]);
	MD5Transform (mdContext->buf, in);

	/* store buffer in digest */
	for (i = 0, ii = 0; i < 4; i++, ii += 4) {
		mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
		mdContext->digest[ii+1] =
			(unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
		mdContext->digest[ii+2] =
			(unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
		mdContext->digest[ii+3] =
			(unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
	}
}

/* Code sequence common to all four rounds.
* evaluates a = b + (a + edi + x + t) <<< s
* Assumes a,b are registers, s,t are immediate constants
* The 'lea' instruction is just a fast way to compute "a = a+t+edi"
*/
#define COM(a,b,x,s,t)\
	__asm     lea a,[t+a+edi] \
	__asm     add a,x \
	__asm     rol a,s \
	__asm     add a,b 

/* Round 1 functions */
/* edi = F(x,y,z) = (x & y) | (~x & z) */
#define F(x,y,z)\
	__asm     mov edi,x \
	__asm     and edi,y \
	__asm     mov esi,x \
	__asm     not esi \
	__asm     and esi,z \
	__asm     or edi,esi

/* a = b + ((a + F(x,y,z) + x + t) <<< s); */
#define FF(a,b,c,d,x,s,t)\
	F(b,c,d) \
	COM(a,b,x,s,t)

/* Round 2 functions */
/* edi = G(x,y,z) = F(z,x,y) = (x & z) | (y & ~z) */
#define G(x,y,z) F(z,x,y)

/* a = b + ((a + G(b,c,d) + x + t) <<< s) */
#define GG(a,b,c,d,x,s,t)\
	G(b,c,d) \
	COM(a,b,x,s,t)

/* Round 3 functions */
/* edi = H(x,y,z) = x ^ y ^ z */
#define H(x,y,z)\
	__asm     mov edi,x \
	__asm     xor edi,y \
	__asm     xor edi,z

/* a = b + ((a + H(b,c,d) + x + t) <<< s) */
#define HH(a,b,c,d,x,s,t)\
	H(b,c,d) \
	COM(a,b,x,s,t)

/* Round 4 functions */
/* edi = I(x,y,z) = y ^ (x | ~z) */
#define I(x,y,z)\
	__asm     mov edi,z \
	__asm     not edi \
	__asm     or edi,x \
	__asm     xor edi,y

/* a = b + ((a + I(b,c,d) + x + t) <<< s) */
#define II(a,b,c,d,x,s,t)\
	I(b,c,d) \
	COM(a,b,x,s,t)

#define A       eax
#define B       ebx
#define C       ecx
#define D       edx

void MD5Transform(UINT4 *buf, UINT4 *input)
{
	/* Save caller's registers */
	__asm     push esi;
	__asm     push edi;

	/* Get buf argument */
	__asm     mov esi,buf;
	__asm     mov A,dword ptr[esi + 0 * 4];        /* A = buf[0] */
	__asm     mov B,dword ptr[esi + 1 * 4];        /* B = buf[1] */
	__asm     mov C,dword ptr[esi + 2 * 4];        /* C = buf[2] */
	__asm     mov D,dword ptr[esi + 3 * 4];        /* D = buf[3] */

	/* Warning: This makes our other args inaccessible until bp
	 * is restored!
	 */
	__asm     push ebp;
	__asm     mov ebp,input

/* Round 1. The *4 factors in the subscripts to bp account for the
 * byte offsets of each long element in the input array.
 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF(A,B,C,D,[ebp + 0*4],S11,3614090360); /* 1 */
	FF(D,A,B,C,[ebp + 1*4],S12,3905402710); /* 2 */
	FF(C,D,A,B,[ebp + 2*4],S13, 606105819); /* 3 */
	FF(B,C,D,A,[ebp + 3*4],S14,3250441966); /* 4 */
	FF(A,B,C,D,[ebp + 4*4],S11,4118548399); /* 5 */
	FF(D,A,B,C,[ebp + 5*4],S12,1200080426); /* 6 */
	FF(C,D,A,B,[ebp + 6*4],S13,2821735955); /* 7 */
	FF(B,C,D,A,[ebp + 7*4],S14,4249261313); /* 8 */
	FF(A,B,C,D,[ebp + 8*4],S11,1770035416); /* 9 */
	FF(D,A,B,C,[ebp + 9*4],S12,2336552879); /* 10 */
	FF(C,D,A,B,[ebp +10*4],S13,4294925233); /* 11 */
	FF(B,C,D,A,[ebp +11*4],S14,2304563134); /* 12 */
	FF(A,B,C,D,[ebp +12*4],S11,1804603682); /* 13 */
	FF(D,A,B,C,[ebp +13*4],S12,4254626195); /* 14 */
	FF(C,D,A,B,[ebp +14*4],S13,2792965006); /* 15 */
	FF(B,C,D,A,[ebp +15*4],S14,1236535329); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG(A,B,C,D,[ebp + 1*4],S21,4129170786); /* 17 */
	GG(D,A,B,C,[ebp + 6*4],S22,3225465664); /* 18 */
	GG(C,D,A,B,[ebp +11*4],S23, 643717713); /* 19 */
	GG(B,C,D,A,[ebp + 0*4],S24,3921069994); /* 20 */
	GG(A,B,C,D,[ebp + 5*4],S21,3593408605); /* 21 */
	GG(D,A,B,C,[ebp +10*4],S22,  38016083); /* 22 */
	GG(C,D,A,B,[ebp +15*4],S23,3634488961); /* 23 */
	GG(B,C,D,A,[ebp + 4*4],S24,3889429448); /* 24 */
	GG(A,B,C,D,[ebp + 9*4],S21, 568446438); /* 25 */
	GG(D,A,B,C,[ebp +14*4],S22,3275163606); /* 26 */
	GG(C,D,A,B,[ebp + 3*4],S23,4107603335); /* 27 */
	GG(B,C,D,A,[ebp + 8*4],S24,1163531501); /* 28 */
	GG(A,B,C,D,[ebp +13*4],S21,2850285829); /* 29 */
	GG(D,A,B,C,[ebp + 2*4],S22,4243563512); /* 30 */
	GG(C,D,A,B,[ebp + 7*4],S23,1735328473); /* 31 */
	GG(B,C,D,A,[ebp +12*4],S24,2368359562); /* 32 */

	/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
	HH(A,B,C,D,[ebp + 5*4],S31,4294588738); /* 33 */
	HH(D,A,B,C,[ebp + 8*4],S32,2272392833); /* 34 */
	HH(C,D,A,B,[ebp +11*4],S33,1839030562); /* 35 */
	HH(B,C,D,A,[ebp +14*4],S34,4259657740); /* 36 */
	HH(A,B,C,D,[ebp + 1*4],S31,2763975236); /* 37 */
	HH(D,A,B,C,[ebp + 4*4],S32,1272893353); /* 38 */
	HH(C,D,A,B,[ebp + 7*4],S33,4139469664); /* 39 */
	HH(B,C,D,A,[ebp +10*4],S34,3200236656); /* 40 */
	HH(A,B,C,D,[ebp +13*4],S31, 681279174); /* 41 */
	HH(D,A,B,C,[ebp + 0*4],S32,3936430074); /* 42 */
	HH(C,D,A,B,[ebp + 3*4],S33,3572445317); /* 43 */
	HH(B,C,D,A,[ebp + 6*4],S34,  76029189); /* 44 */
	HH(A,B,C,D,[ebp + 9*4],S31,3654602809); /* 45 */
	HH(D,A,B,C,[ebp +12*4],S32,3873151461); /* 46 */
	HH(C,D,A,B,[ebp +15*4],S33, 530742520); /* 47 */
	HH(B,C,D,A,[ebp + 2*4],S34,3299628645); /* 48 */

	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II(A,B,C,D,[ebp + 0*4],S41,4096336452); /* 49 */
	II(D,A,B,C,[ebp + 7*4],S42,1126891415); /* 50 */
	II(C,D,A,B,[ebp +14*4],S43,2878612391); /* 51 */
	II(B,C,D,A,[ebp + 5*4],S44,4237533241); /* 52 */
	II(A,B,C,D,[ebp +12*4],S41,1700485571); /* 53 */
	II(D,A,B,C,[ebp + 3*4],S42,2399980690); /* 54 */
	II(C,D,A,B,[ebp +10*4],S43,4293915773); /* 55 */
	II(B,C,D,A,[ebp + 1*4],S44,2240044497); /* 56 */
	II(A,B,C,D,[ebp + 8*4],S41,1873313359); /* 57 */
	II(D,A,B,C,[ebp +15*4],S42,4264355552); /* 58 */
	II(C,D,A,B,[ebp + 6*4],S43,2734768916); /* 59 */
	II(B,C,D,A,[ebp +13*4],S44,1309151649); /* 60 */
	II(A,B,C,D,[ebp + 4*4],S41,4149444226); /* 61 */
	II(D,A,B,C,[ebp +11*4],S42,3174756917); /* 62 */
	II(C,D,A,B,[ebp + 2*4],S43, 718787259); /* 63 */
	II(B,C,D,A,[ebp + 9*4],S44,3951481745); /* 64 */

	__asm pop ebp;             /* We can address our args again */
	__asm mov esi,buf;
	__asm add dword ptr[esi + 0*4],A;    /* buf[0] += A */
	__asm add dword ptr[esi + 1*4],B;    /* buf[1] += B */
	__asm add dword ptr[esi + 2*4],C;    /* buf[2] += C */
	__asm add dword ptr[esi + 3*4],D;    /* buf[3] += D */

	__asm     pop edi;
	__asm     pop esi;
}

void MD5Calc(const UCHAR *pData,ULONG nSize,UCHAR pOut[16])
{
	MD5_CTX ct;
	MD5Init(&ct);
	MD5Update(&ct,(unsigned char *)pData,nSize);
	MD5Final(&ct);

	memcpy(pOut,ct.buf,16);
}
void MD5BinToStr(const UCHAR pMd5[16],CHAR szMd5[32+1])
{
	for(int i = 0;i<16;i++)
	{
		sprintf(&szMd5[i*2],"%02x",pMd5[i]);
	}
	szMd5[32] = '\0';
}