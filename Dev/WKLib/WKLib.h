#pragma once

#define GetMajorVersion(version) ((version)/1000)
#define GetMinorVersion(version) ((version)%1000)

#define DB(x) __asm _emit x
#define DB2(x) DB(x) DB(x)
#define DB4(x) DB2(x) DB2(x)
#define DB8(x) DB4(x) DB4(x)
#define DB16(x) DB8(x) DB8(x)

#ifndef DO_NOT_ENCRYPT
#define ON_NOT_VIP \
	__asm{\
	__asm lea eax,_mark \
	__asm cmp byte ptr [eax],0x90 \
	__asm jz _code_sart \
	__asm jmp _not_decrypt \
	__asm _mark: \
	__asm _emit 0xCC \
	__asm _not_decrypt: \
	__asm push offset _gKeyCheck \
	__asm push offset _mark \
	__asm push offset _code_end \
	__asm push offset _code_sart \
	__asm call DecryptCode \
	__asm cmp eax,TRUE \
	__asm jz _code_sart \
	}

#define ENCRYPT_CODE_BEGIN \
	__asm{ \
	__asm jmp _code_end \
	__asm _code_sart_mark: \
	DB16(0xF0) \
	}\
	__asm _code_sart:

#define ENCRYPT_CODE_END \
	__asm _code_end: \
	__asm{ \
	__asm _code_end_mark: \
	DB(0xEB) DB(0x0E) \
	DB8(0xE0) \
	DB4(0xE0) \
	DB2(0xE0) \
	}
#else
	#define ON_NOT_VIP __asm jmp _code_sart
	#define ENCRYPT_CODE_BEGIN __asm _code_sart:
	#define ENCRYPT_CODE_END
#endif

#define KEY_CHECK_LOCATOR_VALUE 0x17,0x87,0xCD,0x67,0xC8,0xC8,0x52,0x43,0x32,0xD2,0x9C,0x6D,0xDB,0xA3,0x1,0xF5
#define KeyCheckVersion 1
struct KeyCheck
{
	UCHAR m_locator[16];
	ULONG m_minVer;
	ULONG m_version;
	ULONG m_licence;
	UCHAR m_raw[16];
	UCHAR m_sum[16];
};

extern KeyCheck _gKeyCheck;//defined in App

ULONG __stdcall GetWKLibVersion();
void __stdcall VIPTip(HWND hWnd,LPCSTR lpMsg);
BOOL __stdcall UpdateLicence();
BOOL __stdcall DecryptCode(UCHAR *pCodeStart,UCHAR *pCodeEnd,UCHAR *pMark,const KeyCheck *pKC);