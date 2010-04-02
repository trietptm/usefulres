#pragma once

/* typedef a 32-bit type */
typedef unsigned long int UINT4;

/* Data structure for MD5 (Message-Digest) computation */
typedef struct {
	UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                 /* scratch buffer */
	unsigned char in[64];         /* input buffer */
	unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(MD5_CTX *);
void MD5Transform(UINT4 *, UINT4 *);

void MD5Calc(const UCHAR *pData,ULONG nSize,UCHAR pOut[16]);
void MD5BinToStr(const UCHAR pMd5[16],CHAR szMd5[32+1]);