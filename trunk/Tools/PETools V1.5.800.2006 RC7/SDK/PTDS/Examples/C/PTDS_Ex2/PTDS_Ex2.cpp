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

#pragma warning(disable:4018)

HWND       hWndPTDS;
DWORD      dwMyPID;

CONST CHAR szCaption[]   = "PE Tools PlugIn";
CONST CHAR szDone[]      = "Dump successfully saved!";
CONST CHAR szError[]     = "An unknown error has occurred!";
CONST CHAR cPluginName[] = "[Dump Full] - PlugIn example for C by NEOx <neox@uinc.ru>";


BOOL SaveBufferToDisk(IN CHAR *szFilePath, IN LPVOID lpBuffer, IN DWORD dwSize);
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

	PTDSLog(hWndPTDS, (CHAR *)cPluginName, sizeof(cPluginName));

	DWORD dwPids[60];
	PTDS_ENUM_PIDS ptdsEnumPID;
	ZeroMemory(&ptdsEnumPID, sizeof(ptdsEnumPID));
	ptdsEnumPID.dwStructSize = sizeof(ptdsEnumPID);
	ptdsEnumPID.pdwPIDChain  = (PDWORD)&dwPids;
	ptdsEnumPID.dwChainSize  = sizeof(dwPids);
	SendMessage(hWndPTDS, WM_PTDS_CMD_ENUMPROCESSIDS, dwMyPID, (LPARAM)&ptdsEnumPID);

	for(int i = 0; i < ptdsEnumPID.dwItemCount; i++)
	{
		wsprintf(cBuff, "ProcessID #%0.2d: 0x%0.8X", i + 1, dwPids[i]);
		PTDSLog(hWndPTDS, cBuff, 100);
	}

	PTDS_MODULE_INFO ptdsModInfo;
	ZeroMemory(&ptdsModInfo, sizeof(ptdsModInfo));
	ptdsModInfo.dwStructSize = sizeof(ptdsModInfo);
	ptdsModInfo.dwPID        = dwMyPID;
	SendMessage(hWndPTDS, WM_PTDS_CMD_QUERYPROCESSMODULEINFO, dwMyPID, (LPARAM)&ptdsModInfo);

	wsprintf(cBuff, "Path: %s", ptdsModInfo.cModulePath);
	PTDSLog(hWndPTDS, cBuff, 100);
	wsprintf(cBuff, "Image Base: 0x%0.8X | Image Size: 0x%0.8X", ptdsModInfo.hImageBase, ptdsModInfo.dwImageSize);
	PTDSLog(hWndPTDS, cBuff, 100);

	DWORD dwMids[60];
	PTDS_ENUM_PROCESS_MODULES ptdsEnumMID;
	ZeroMemory(&ptdsEnumMID, sizeof(ptdsEnumMID));
	ptdsEnumMID.dwStructSize       = sizeof(ptdsEnumMID);
	ptdsEnumMID.pdwImageBaseChain  = (PDWORD)&dwMids;
	ptdsEnumMID.dwChainSize        = sizeof(dwMids);
	ptdsEnumMID.dwPID              = dwMyPID;
	SendMessage(hWndPTDS, WM_PTDS_CMD_ENUMPROCESSMODULES, dwMyPID, (LPARAM)&ptdsEnumMID);

	for(int k = 0; k < ptdsEnumMID.dwItemCount; k++)
	{
		wsprintf(cBuff, "ModuleID #%0.2d: 0x%0.8X", k + 1, dwMids[k]);
		PTDSLog(hWndPTDS, cBuff, 100);
	}
	
	
	DWORD dwWittenBytes = 0;
	PTDS_FULL_DUMP ptdsFullDump;
	ZeroMemory(&ptdsFullDump,   sizeof(ptdsFullDump));
	ptdsFullDump.dwStructSize = sizeof(ptdsFullDump);
	ptdsFullDump.dwPID        = dwMyPID;
	ptdsFullDump.hModuleBase  = 0/* (HINSTANCE)dwMids[2]*/;
	ptdsFullDump.dwFlags      = PTDS_REB_REBUILDIMAGE/* | PTDS_DUMP_SAVEVIAOFN*/;
	SendMessage(hWndPTDS, WM_PTDS_CMD_DUMPPROCESSMODULE, dwMyPID, (LPARAM)&ptdsFullDump);

	if(ptdsFullDump.bDumpSuccessfully == TRUE)
	{
		if(SaveBufferToDisk("C:\\dumped.exe", ptdsFullDump.pDumpedImage, ptdsFullDump.dwSizeOfDumpedImage))
		{
			MessageBox(GetActiveWindow(), szDone, szCaption, MB_OK | MB_ICONINFORMATION);
			return 0;
		}
		else
		{
			MessageBox(GetActiveWindow(), szError, szCaption, MB_OK | MB_ICONERROR);
			return -1;
		}
	}

	return 0;
}

BOOL SaveBufferToDisk(IN CHAR *szFilePath, IN LPVOID lpBuffer, IN DWORD dwSize)
{
	DWORD dwBytesWritten = 0;

	HANDLE hFile = CreateFile(szFilePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE; // Error

	BOOL bError = (!WriteFile(hFile, lpBuffer, dwSize, &dwBytesWritten, NULL) || dwBytesWritten != dwSize);

	// Error?
	if(bError)
	{
		// Clean up
		CloseHandle(hFile);
		return FALSE; // Error
	}

	// Clean up
	CloseHandle(hFile);
	return TRUE; // No errors
}

VOID PTDSLog(HWND hWnd, CHAR *szString, DWORD dwStringSize)
{
	PTDS_INFO_LOG PTInfoLog;
	PTInfoLog.dwStructSize  = sizeof(PTInfoLog);
	PTInfoLog.szStr         = szString;
	PTInfoLog.dwStrSize     = dwStringSize;
	SendMessage(hWnd, WM_PTDS_CMD_ADDLOG, dwMyPID, (LPARAM)&PTInfoLog);
}