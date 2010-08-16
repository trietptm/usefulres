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

HWND       hWndPTDS;
DWORD      dwMyPID;
CONST CHAR szCaption[]   = "PE Tools PlugIn";
CONST CHAR cPluginName[] = "[PID Search] - PlugIn example for C by NEOx <neox@uinc.ru>";

VOID PTDSLog(HWND hWnd, CHAR *szString, DWORD dwStringSize);

int main()
{
	CHAR cBuff[MAX_PATH] = {0};
	dwMyPID  = GetCurrentProcessId();
	hWndPTDS = FindWindow(NULL, PTDS_WND_NAME);
	if(!hWndPTDS)
	{
		MessageBox(NULL, "Launch \"PE Tools Dumper Server\" first !", szCaption, MB_OK | MB_ICONERROR);
		return -1;
	}

	PTDSLog(hWndPTDS, (char *)cPluginName, sizeof(cPluginName));

	PTDS_VERSION ptdsVer;
	ptdsVer.dwStructSize = sizeof(ptdsVer);
    ptdsVer.dwVersion    = 0;
    SendMessage(hWndPTDS, WM_PTDS_CMD_QUERYVERSION, (WPARAM)dwMyPID, (LPARAM)&ptdsVer);

	wsprintf(cBuff, "Interface version is: %d.%d", HIWORD(ptdsVer.dwVersion), LOWORD(ptdsVer.dwVersion));
	PTDSLog(hWndPTDS, cBuff, sizeof(cBuff));

	PTDS_FIND_PID ptdsFindPid;
	ptdsFindPid.dwStructSize = sizeof(ptdsFindPid);
    ptdsFindPid.dwPID        = 0;
    GetModuleFileName(0, ptdsFindPid.cProcessPath, sizeof(ptdsFindPid));
    SendMessage(hWndPTDS, WM_PTDS_CMD_FINDPROCESSID, (WPARAM)dwMyPID, (LPARAM)&ptdsFindPid);

	if(ptdsFindPid.dwPID == 0)
	{
		CHAR szErr[] = "Searched my PID with the module path...not found !!!";
		PTDSLog(hWndPTDS, szErr, sizeof(szErr));
	}
	else
	{
		wsprintf(cBuff, "Searched my PID with the module path...found: 0x%0.8X", ptdsFindPid.dwPID);
		PTDSLog(hWndPTDS, cBuff, sizeof(cBuff));
	}

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