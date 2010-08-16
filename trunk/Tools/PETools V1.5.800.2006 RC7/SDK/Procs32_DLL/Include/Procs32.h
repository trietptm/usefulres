/*=======================================================================*\
|| ##################################################################### ||
|| # Procs32.h                                                         # ||
|| # Version 1.5.70.2005                                               # ||
|| # ----------------------------------------------------------------- # ||
|| # Coded by NEOx <neox@pisem.net>                                    # ||
|| # Copyright © 2005 Underground InformatioN Center                   # ||
|| # http://www.uinc.ru                                                # ||
|| ##################################################################### ||
\*=======================================================================*/

#ifndef __Procs32_h__
#define __Procs32_h__

//#ifdef _MSC_VER
//#pragma comment(lib, "Procs32.lib")
//#endif

#ifdef PROCS32_EXPORTS
#define PROCS32_API __declspec(dllexport)__stdcall
#else
#define PROCS32_API __declspec(dllimport)__stdcall
#endif

// Force byte alignment of structures
#if defined(__BORLANDC__)
#pragma option -a1
#else if defined(_MSC_VER)
#pragma pack(1)
#endif

// Structs
typedef struct _PROCESS_ENTRY
{
	CHAR  lpFileName[MAX_PATH];
	DWORD dwPID;
	BOOL  bSystemProcess;
} PROCESS_ENTRY, *PPROCESS_ENTRY;

typedef struct _MODULE_ENTRY
{
	CHAR  lpFileName[MAX_PATH];
	DWORD dwImageBase;
	DWORD dwImageSize;
	BOOL  bSystemProcess;
} MODULE_ENTRY, *PMODULE_ENTRY;

#if defined(_MSC_VER)
#pragma pack()
#endif 

// Exports
BOOL   PROCS32_API GetProcessFirst(OUT PPROCESS_ENTRY pEntry);
BOOL   PROCS32_API GetProcessNext(OUT PPROCESS_ENTRY pEntry);
BOOL   PROCS32_API GetModuleFirst(IN DWORD dwPID, OUT PMODULE_ENTRY mEntry);
BOOL   PROCS32_API GetModuleNext(IN DWORD dwPID, OUT PMODULE_ENTRY mEntry);
DWORD  PROCS32_API GetNumberOfProcesses(VOID);
DWORD  PROCS32_API GetNumberOfModules(IN DWORD dwPID);
BOOL   PROCS32_API GetProcessPath(IN DWORD dwPID, OUT CHAR *szBuff);
BOOL   PROCS32_API GetProcessBaseSize(IN DWORD dwPID, OUT DWORD *dwImageBase, OUT DWORD *dwImageSize);
HANDLE PROCS32_API GetModuleHandleEx(IN DWORD dwPID, OUT CHAR *szModule);
DWORD  PROCS32_API GetProcessPathID(IN CHAR *szPath);
BOOL   PROCS32_API IsProcessRunned(IN DWORD dwPID);
BOOL   PROCS32_API GetProcessIDList(OUT DWORD *dwPIDArray, IN DWORD dwArraySize);
BOOL   PROCS32_API GetModuleHandleList(IN DWORD dwPID, OUT DWORD *dwHandleArray, IN DWORD dwArraySize);
BOOL   PROCS32_API GetModuleImageSize(IN DWORD dwPID, IN HMODULE hModule, OUT DWORD *dwImageSize);

#endif // __Procs32_h__