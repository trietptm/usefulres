////////////////////////////////////////////////////////////////////////////////
// Procs32.h
//
// Version 1.3
//
// Coded by NEOx <NEOx@PIsem.net>
//
// Copyright [c] 2002, Underground InformatioN Center
////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "Procs32.lib")

const int MAX_FILENAME = 256;
struct ProcessEntry
{
	LPTSTR lpFileName;
    DWORD  dwPID;
    WORD   hTask16;
	BOOL   bSystemProcess;
    ProcessEntry() : dwPID(0)
    {lpFileName = new TCHAR[MAX_FILENAME];}
    ProcessEntry(ProcessEntry &e) : dwPID(e.dwPID)
    {strcpy(lpFileName, e.lpFileName);}
    virtual ~ProcessEntry()
    {delete[] lpFileName;}
};

struct ModuleEntry
{
    LPTSTR lpFileName;
    PVOID  pLoadBase;
	DWORD  dwImageSize;
	BOOL   bSystemProcess;
    ModuleEntry() : pLoadBase(NULL), dwImageSize(NULL)
    {lpFileName = new TCHAR[MAX_FILENAME];}
    ModuleEntry(ModuleEntry &e) : pLoadBase(e.pLoadBase), dwImageSize(e.dwImageSize)
    {strcpy(lpFileName, e.lpFileName);}
    virtual ~ModuleEntry()
    {delete[] lpFileName;}
};

BOOL   __stdcall GetProcessFirst(ProcessEntry *pEntry);
BOOL   __stdcall GetProcessNext(ProcessEntry *pEntry);
BOOL   __stdcall GetModuleFirst(DWORD dwPID, ModuleEntry *mEntry);
BOOL   __stdcall GetModuleNext(DWORD dwPID, ModuleEntry *mEntry);
DWORD  __stdcall GetNumberOfProcesses(VOID);
DWORD  __stdcall GetNumberOfModules(DWORD dwPID);
BOOL   __stdcall GetProcessPath(DWORD dwPID, LPSTR szBuff);
BOOL   __stdcall GetProcessBaseSize(DWORD dwPID, DWORD *dwImageBase, DWORD *dwImageSize);
HANDLE __stdcall GetModuleHandleEx(DWORD dwPID, char* szModule);
DWORD  __stdcall GetProcessPathID(char* szPath);