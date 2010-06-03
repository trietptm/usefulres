#pragma once
#include <Windows.h>
#include <atlstr.h>

#ifndef _SHA1_H_
#define _SHA1_H_

/**//* sha1.h */

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
	shaSuccess = 0,
		shaNull,            /**//* Null pointer parameter */
		shaInputTooLong,    /**//* input data too long */
		shaStateError       /**//* called Input after Result */
};
#endif

#define SHA1HashSize 20

typedef struct SHA1Context
{
	DWORD Intermediate_Hash[SHA1HashSize/4]; // Message Digest

	DWORD Length_Low;            // Message length in bits
	DWORD Length_High;            // Message length in bits

	int Message_Block_Index;    // Index into message block array
	unsigned char Message_Block[64];    // 512-bit message blocks

	int Computed;                // Is the digest computed?
	int Corrupted;                // Is the message digest corrupted?
} SHA1Context;

// Function Prototypes 
CString GetSHA1String(CString sSource);

int SHA1Reset(SHA1Context *);
int SHA1Input(SHA1Context *, const unsigned char *, unsigned int);
int SHA1Result(SHA1Context *, unsigned char Message_Digest[SHA1HashSize]);
CString Sha1toBase32(const unsigned char *);

#endif