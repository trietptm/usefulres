/////////////////////////////////////////////////////////////////
//
// PE Tools Dumper Server - PlugIn
//
// Coded by NEOx <neox@pisem.net>
//
/////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include "..\..\..\Include\PTDS.h"

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/IGNORE:4078 /IGNORE:4089")
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/merge:.rdata=.data")
#pragma comment(linker,"/merge:.text=.data")
#pragma comment(linker,"/merge:.reloc=.data")
#if _MSC_VER >= 1000
#pragma comment(linker,"/FILEALIGN:0x200")
#endif
#endif
#pragma comment(linker,"/ENTRY:main")


#define SAVE_VIA_OFN  // Save image to disk?


HWND       hWndPTDS;
DWORD      dwMyPID;

CONST CHAR szCaption[]   = "PE Tools PlugIn";
CONST CHAR szDone[]      = "Dump successfully saved!";
CONST CHAR szError[]     = "An unknown error has occurred!";
CONST CHAR cPluginName[] = "[Partial dumper] - PlugIn example for C by NEOx <neox@uinc.ru>";

VOID PTDSLog(HWND hWnd, CHAR *szString, DWORD dwStringSize);

int main()
{
	dwMyPID  = GetCurrentProcessId();
	hWndPTDS = FindWindow(NULL, PTDS_WND_NAME);
	if(!hWndPTDS)
	{
		MessageBox(NULL, "Launch \"PE Tools Dumper Server\" first !", szCaption, MB_OK | MB_ICONERROR);
		return -1;
	}

	PTDSLog(hWndPTDS, (char *)cPluginName, sizeof(cPluginName));

	PTDS_PARTIAL_DUMP ptdsPatialDump = {0};
	ptdsPatialDump.dwStructSize = sizeof(ptdsPatialDump);
    ptdsPatialDump.dwPID        = dwMyPID;

#ifdef SAVE_VIA_OFN
	ptdsPatialDump.bSaveViaOfn  = TRUE;
#else
	ptdsPatialDump.bSaveViaOfn  = FALSE;
#endif

	ptdsPatialDump.dwBlockStartAddress = (DWORD)GetModuleHandle(NULL);
	ptdsPatialDump.dwBlockSize = sizeof(IMAGE_DOS_HEADER);

    SendMessage(hWndPTDS, WM_PTDS_CMD_DUMPPROCESSBLOCK, (WPARAM)dwMyPID, (LPARAM)&ptdsPatialDump);

#ifdef SAVE_VIA_OFN
	if(ptdsPatialDump.bUserSaved)
	{
		MessageBox(GetActiveWindow(), szDone, szCaption, MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		MessageBox(GetActiveWindow(), szError, szCaption, MB_OK | MB_ICONERROR);
	}
#else
	if(ptdsPatialDump.bDumpSuccessfully)
	{
		MessageBox(GetActiveWindow(), szDone, szCaption, MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		MessageBox(GetActiveWindow(), szError, szCaption, MB_OK | MB_ICONERROR);
	}
#endif

	return 0;
}

VOID PTDSLog(HWND hWnd, CHAR *szString, DWORD dwStringSize)
{
	PTDS_INFO_LOG PTInfoLog;
	PTInfoLog.dwStructSize  = sizeof(PTInfoLog);
	PTInfoLog.szStr         = szString;
	PTInfoLog.dwStrSize     = dwStringSize;
	SendMessage(hWnd, WM_PTDS_CMD_ADDLOG, dwMyPID, (LPARAM)&PTInfoLog);
}