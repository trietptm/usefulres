// Procs_Ex1.cpp : Defines the entry point for the application.
// Coded by NEOx <NEOx@Pisem.net>
///////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include "..\..\Include\Procs32.h"

#pragma comment(lib,"..\\..\\Procs32.lib") 

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/IGNORE:4078 /IGNORE:4089")
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/merge:.rdata=.data")
#pragma comment(linker,"/merge:.text=.data")
#pragma comment(linker,"/merge:.reloc=.data")
#if _MSC_VER >= 1000
#pragma comment(linker,"/ENTRY:WinMain")
#pragma comment(linker,"/FILEALIGN:0x200")
#endif
#endif

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	CHAR szTmp[MAX_PATH];
	///////////////////////////////////////////////////////////////////////////////////////////
	// Testing
	// Procs32.dll!GetProcessPath
	///////////////////////////////////////////////////////////////////////////////////////////
	LPSTR szPath[MAX_PATH];
	GetProcessPath(GetCurrentProcessId(), (char *)szPath);
	MessageBox(NULL, (const char *)szPath, "GetProcessPath", MB_OK | MB_ICONINFORMATION);
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	// Testing
	// Procs32.dll!GetNumberOfProcesses
	///////////////////////////////////////////////////////////////////////////////////////////
	wsprintf(szTmp, "Number of processes: %d", GetNumberOfProcesses());
	MessageBox(NULL, (const char *)szTmp, "GetNumberOfProcesses", MB_OK | MB_ICONINFORMATION);
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	// Testing
	// Procs32.dll!GetNumberOfModules
	///////////////////////////////////////////////////////////////////////////////////////////
	wsprintf(szTmp, "Number of modules: %d", GetNumberOfModules(GetCurrentProcessId()));
	MessageBox(NULL, (const char *)szTmp, "GetNumberOfModules", MB_OK | MB_ICONINFORMATION);
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	// Testing
	// Procs32.dll!GetProcessBaseSize
	///////////////////////////////////////////////////////////////////////////////////////////
	DWORD dwImageBase, dwImageSize;
	GetProcessBaseSize(GetCurrentProcessId(), &dwImageBase, &dwImageSize);
	wsprintf(szTmp, "ImageBase = 0x%0.8X\nImageSize = 0x%0.8X", dwImageBase, dwImageSize);
	MessageBox(NULL, (const char *)szTmp, "GetProcessBaseSize", MB_OK | MB_ICONINFORMATION);
	///////////////////////////////////////////////////////////////////////////////////////////

	return 0;
}