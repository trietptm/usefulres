#include "stdafx.h"
#include "sha1.h"

// Define the SHA1 circular left shift macro 
#define SHA1CircularShift(bits, word) (((word) << (bits)) | ((word) >> (32-(bits))))

// Local Function Prototyptes */

void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

int SHA1Reset(SHA1Context *c)
{
	if (!c)
		return shaNull;

	c->Length_Low             = 0;
	c->Length_High            = 0;
	c->Message_Block_Index    = 0;

	c->Intermediate_Hash[0]   = 0x67452301;
	c->Intermediate_Hash[1]   = 0xEFCDAB89;
	c->Intermediate_Hash[2]   = 0x98BADCFE;
	c->Intermediate_Hash[3]   = 0x10325476;
	c->Intermediate_Hash[4]   = 0xC3D2E1F0;

	c->Computed   = 0;
	c->Corrupted  = 0;

	return shaSuccess;
}

int SHA1Result( SHA1Context *c, unsigned char Message_Digest[SHA1HashSize])
{
	int i;

	if (!c || !Message_Digest)
		return shaNull;

	if (c->Corrupted)
		return c->Corrupted;

	if (!c->Computed)
		{
			SHA1PadMessage(c);
			for(i = 0; i<64; c->Message_Block[++i] = 0)
				;
			c->Length_Low = 0;    /**//* and clear length */
			c->Length_High = 0;
			c->Computed = 1;

		}

		for(i = 0; i < SHA1HashSize; ++i)
			Message_Digest[i] = c->Intermediate_Hash[i>>2] >> 8 * (3 - (i & 0x03));

		return shaSuccess;
}

int SHA1Input(SHA1Context *context, const unsigned char *message_array, unsigned length)
{
	if (!length)
		return shaSuccess;

	if (!context || !message_array)
		return shaNull;

	if (context->Computed)
		{
			context->Corrupted = shaStateError;
			return shaStateError;
		}

		if (context->Corrupted)
			return context->Corrupted;

		while(length-- && !context->Corrupted)
			{
				context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);
				context->Length_Low += 8;
				if (context->Length_Low == 0)
					{
						context->Length_High++;
						if (context->Length_High == 0)
							{
								/**//* Message is too long */
								context->Corrupted = 1;
							}
					}

					if (context->Message_Block_Index == 64)
						SHA1ProcessMessageBlock(context);

					message_array++;
			}

			return shaSuccess;
}

void SHA1ProcessMessageBlock(SHA1Context *context)
{
	const DWORD K[] =    { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
	int   t;
	DWORD temp;
	DWORD W[80];
	DWORD A, B, C, D, E;

	/**//*
		*  Initialize the first 16 words in the array W
		*/
	for(t = 0; t < 16; t++)
	{
		W[t]  = context->Message_Block[t * 4] << 24;
		W[t] |= context->Message_Block[t * 4 + 1] << 16;
		W[t] |= context->Message_Block[t * 4 + 2] << 8;
		W[t] |= context->Message_Block[t * 4 + 3];
	}

	for(t = 16; t < 80; t++)
		W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);

	A = context->Intermediate_Hash[0];
	B = context->Intermediate_Hash[1];
	C = context->Intermediate_Hash[2];
	D = context->Intermediate_Hash[3];
	E = context->Intermediate_Hash[4];

	for(t = 0; t < 20; t++)
	{
		temp =  SHA1CircularShift(5,A) +
			((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);

		B = A;
		A = temp;
	}

	for(t = 20; t < 40; t++)
	{
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for(t = 40; t < 60; t++)
	{
		temp = SHA1CircularShift(5,A) +
			((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for(t = 60; t < 80; t++)
	{
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	context->Intermediate_Hash[0] += A;
	context->Intermediate_Hash[1] += B;
	context->Intermediate_Hash[2] += C;
	context->Intermediate_Hash[3] += D;
	context->Intermediate_Hash[4] += E;

	context->Message_Block_Index = 0;
}


void SHA1PadMessage(SHA1Context *context)
{
	if (context->Message_Block_Index > 55)
		{
			context->Message_Block[context->Message_Block_Index++] = 0x80;
			while(context->Message_Block_Index < 64)
				context->Message_Block[context->Message_Block_Index++] = 0;

			SHA1ProcessMessageBlock(context);

			while(context->Message_Block_Index < 56)
				context->Message_Block[context->Message_Block_Index++] = 0;
		}
	else
		{
			context->Message_Block[context->Message_Block_Index++] = 0x80;
			while(context->Message_Block_Index < 56)
				context->Message_Block[context->Message_Block_Index++] = 0;
		}

		/**//*
			*  Store the message length as the last 8 octets
			*/
		context->Message_Block[56] = context->Length_High >> 24;
		context->Message_Block[57] = context->Length_High >> 16;
		context->Message_Block[58] = context->Length_High >> 8;
		context->Message_Block[59] = context->Length_High;
		context->Message_Block[60] = context->Length_Low >> 24;
		context->Message_Block[61] = context->Length_Low >> 16;
		context->Message_Block[62] = context->Length_Low >> 8;
		context->Message_Block[63] = context->Length_Low;

		SHA1ProcessMessageBlock(context);
}

// Convert 5 Bytes to 8 Bytes Base32
void _Sha1toBase32(unsigned char *out, const unsigned char *in)
{
	const char *Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

	out[0] = Table[((in[0] >> 3)               ) & 0x1F];
	out[1] = Table[((in[0] << 2) | (in[1] >> 6)) & 0x1F];
	out[2] = Table[((in[1] >> 1)               ) & 0x1F];
	out[3] = Table[((in[1] << 4) | (in[2] >> 4)) & 0x1F];
	out[4] = Table[((in[2] << 1) | (in[3] >> 7)) & 0x1F];
	out[5] = Table[((in[3] >> 2)               ) & 0x1F];
	out[6] = Table[((in[3] << 3) | (in[4] >> 5)) & 0x1F];
	out[7] = Table[((in[4]     )               ) & 0x1F];
}

// Return a base32 representation of a sha1 hash
CString Sha1toBase32(const unsigned char *Sha1)
{
	char Base32[32];
	CString ret;

	_Sha1toBase32((unsigned char *)Base32, Sha1);
	_Sha1toBase32((unsigned char *)Base32 + 8, Sha1 + 5);
	_Sha1toBase32((unsigned char *)Base32 + 16, Sha1 + 10);
	_Sha1toBase32((unsigned char *)Base32 + 24, Sha1 + 15);

	ret = CString(Base32, 32);
	return ret;
}

CString GetSHA1String(CString sSource)
{
	SHA1Context context;
	unsigned char digest[20];
	CString sTmp, sRet;
	SHA1Reset(&context);
	SHA1Input(&context, (const BYTE*)sSource.GetBuffer(0), sSource.GetLength());
	SHA1Result(&context,digest);
	for (int i = 0; i < 20; i++) {
		sTmp.Format("%02X", digest[i]);
		sRet += sTmp;
	}
	return sRet;
}