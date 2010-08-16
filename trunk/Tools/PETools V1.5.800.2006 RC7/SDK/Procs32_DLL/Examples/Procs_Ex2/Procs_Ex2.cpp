// Procs_Ex2.cpp : Defines the entry point for the console application.
// Coded by NEOx <NEOx@Pisem.net>
///////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>
#include <windows.h>
#include "..\..\Include\Procs32.h"

#pragma comment(lib,"..\\..\\Procs32.lib")

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/IGNORE:4078 /IGNORE:4089")
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/merge:.rdata=.data")
#pragma comment(linker,"/merge:.text=.data")
#if _MSC_VER >= 1000
#pragma comment(linker,"/FILEALIGN:0x200")
#endif
#endif

void PrintText(char *szText);
BOOL EnableDebugPrivilege(BOOL fEnable);

#define IsNT (BOOL)(GetVersion() < 0x80000000 ? TRUE : FALSE)

int main(int argc, char* argv[])
{
	PROCESS_ENTRY pEntry;
	MODULE_ENTRY mEntry;
	CHAR szTmp[MAX_PATH + MAX_PATH];

	if(IsNT)
	{
		EnableDebugPrivilege(TRUE);
		PrintText("SeDebugPrivilege was not enabled !!!\r\n");
	}

	PrintText("-===< Procs32 Example v1.0 >===-\r\n");
	PrintText("Coded by NEOx <NEOx@Pisem.net>\r\n");
	PrintText("Copyright [c] 2002, Underground InformatioN Center\r\n\r\n");

	for(BOOL pOK = GetProcessFirst(&pEntry); pOK; pOK = GetProcessNext(&pEntry))
	{
		wsprintf(szTmp, "%s\r\n", pEntry.lpFileName);
		PrintText(szTmp);
		for(BOOL mOK = GetModuleFirst(pEntry.dwPID, &mEntry); mOK; mOK = GetModuleNext(pEntry.dwPID, &mEntry))
		{
			wsprintf(szTmp, "    - %s %0.8X - %0.8X\r\n", mEntry.lpFileName, mEntry.dwImageBase, mEntry.dwImageSize);
			PrintText(szTmp);
		}
	}
	
	PrintText("End of list\r\n");
	return 0;
}

void PrintText(char *szText)
{
	_lwrite(STD_OUTPUT_HANDLE, szText, lstrlen(szText));
	return;
}

BOOL EnableDebugPrivilege(BOOL fEnable)
{
	HANDLE hToken;
	BOOL   fOk = FALSE;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return fOk;
}