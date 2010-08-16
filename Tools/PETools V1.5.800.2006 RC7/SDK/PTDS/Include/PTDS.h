//////////////////////////////////////////////////////////////////////////////
//
// Include file for PE Tools Dumper Server v0.7 (C/C++)
// ------------------------------------------------------------
//
// Version: 0.7
//
// Coded by NEOx <neox@pisem.net>
//
// Copyright [c] 2005, Underground InformatioN Center
//
// Visit us http://www.uinc.ru/
//////////////////////////////////////////////////////////////////////////////

#ifndef __PTDS_h__
#define __PTDS_h__

#include <windows.h>

//
// Constants
//
#define PTDS_INTERFACE_VERSION              0x00000007       // 0.7
#define PTDS_WND_NAME                       "PE Tools Dumper Server"

//
// Commands
//
#define WM_PTDS_CMD_FIRST                   WM_USER           + 0x6000

// wParam - Client PID
// lParam - PPTDS_INFO_LOG
#define WM_PTDS_CMD_ADDLOG                  WM_PTDS_CMD_FIRST + 10

// wParam - Client PID
// lParam - PPTDS_VERSION
#define WM_PTDS_CMD_QUERYVERSION            WM_PTDS_CMD_FIRST + 20

// wParam - Client PID
// lParam - PPTDS_ENUM_PIDS
#define WM_PTDS_CMD_ENUMPROCESSIDS          WM_PTDS_CMD_FIRST + 30

// wParam - Client PID
// lParam - PPTDS_ENUM_PROCESSMODULES
#define WM_PTDS_CMD_ENUMPROCESSMODULES      WM_PTDS_CMD_FIRST + 40

// wParam - Client PID
// lParam - PPTDS_MODULE_INFO
#define WM_PTDS_CMD_QUERYPROCESSMODULEINFO  WM_PTDS_CMD_FIRST + 50

// wParam - Client PID
// lParam - PPTDS_FIND_PID
#define WM_PTDS_CMD_FINDPROCESSID           WM_PTDS_CMD_FIRST + 60

// wParam - Client PID
// lParam - PPTDS_FULL_DUMP
#define WM_PTDS_CMD_DUMPPROCESSMODULE       WM_PTDS_CMD_FIRST + 70

// wParam - Client PID
// lParam - PPTDS_PARTIAL_DUMP
#define WM_PTDS_CMD_DUMPPROCESSBLOCK        WM_PTDS_CMD_FIRST + 80

//
// Dump options
//
#define PTDS_DUMP_CORRECTIMAGESIZE          0x00000001
#define PTDS_DUMP_PASTEHEADERFROMDISK       0x00000002
#define PTDS_DUMP_FORCERAWMODE              0x00000004
#define PTDS_DUMP_SAVEVIAOFNDLG             0x00000008

//
// Rebuilding options
//
#define PTDS_REB_REBUILDIMAGE               0x00010000
#define PTDS_REB_WIPERELOCATION             0x00020000
#define PTDS_REB_VALIDATEIMAGE              0x00040000
#define PTDS_REB_CHANGEIMAGEBASE            0x00080000
//#define PTDS_REB_REBUILDIMPORTS             0x00100000 // Reserved (do not use)
//#define PTDS_REB_REBUILDRESOURCE            0x00200000 // Reserved (do not use)

//
// Structures
//

// Force byte alignment of structures
#if defined(__BORLANDC__)
#pragma option -a1
#else if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct _PTDS_INFO_LOG
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_INFO_LOG)
	IN     CHAR*         szStr;                                    // String
	IN     DWORD         dwStrSize;                                // Terminated NULL-character
} PTDS_INFO_LOG, *PPTDS_INFO_LOG;

typedef struct _PTDS_VERSION
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_VERSION)
	OUT    DWORD         dwVersion;
} PTDS_VERSION, *PPTDS_VERSION;

typedef struct _PTDS_ENUM_PIDS
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_ENUM_PIDS)
	IN     DWORD         dwChainSize;                              // Array size
	OUT    PDWORD        pdwPIDChain;                              // DWORD array for PIDs
	OUT    DWORD         dwItemCount;
} PTDS_ENUM_PIDS, *PPTDS_ENUM_PIDS;

typedef struct _PTDS_ENUM_PROCESS_MODULES
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_ENUM_PROCESS_MODULES)
	IN     DWORD         dwPID;
	IN     DWORD         dwChainSize;                              // Array size
	OUT    PDWORD        pdwImageBaseChain;                        // DWORD array for module handles
	OUT    DWORD         dwItemCount;
} PTDS_ENUM_PROCESS_MODULES, *PPTDS_ENUM_PROCESS_MODULES;

typedef struct _PTDS_MODULE_INFO
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_MODULE_INFO)
	IN     DWORD         dwPID;
	IN     HMODULE       hImageBase;                               // if NULL - calling module is dumped
	OUT    DWORD         dwImageSize;
	OUT    CHAR          cModulePath[MAX_PATH];
} PTDS_MODULE_INFO, *PPTDS_MODULE_INFO;

typedef struct _PTDS_FIND_PID
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_FIND_PID)
	IN     CHAR          cProcessPath[MAX_PATH];
	OUT    DWORD         dwPID;
} PTDS_FIND_PID, *PPTDS_FIND_PID;

typedef struct _PTDS_FULL_DUMP
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_FULL_DUMP)
	IN     DWORD         dwFlags;                                  // PTDS_DUMP_XXX/PTDS_REB_XXX
	IN     DWORD         dwPID;
	IN     HMODULE       hModuleBase;                              // NULL - calling module is dumped
	OUT    BOOL          bDumpSuccessfully;
	OUT    DWORD         dwSizeOfDumpedImage;
	OUT    BOOL          bUserSaved;                               // PTDS_DUMP_SAVEVIAOFNDLG
	OUT    CHAR          cSavedTo[MAX_PATH];                       // PTDS_DUMP_SAVEVIAOFNDLG
	OUT    LPVOID        pDumpedImage;                             // !PTDS_DUMP_SAVEVIAOFNDLG
	// rebuilding structure elements
	IN     DWORD         dwNewImageBase;                           // format: 0xXXXX0000
	IN     DWORD         dwEntryPointRva;                          // 0 = Don't set to image
	IN     DWORD         dwExportTableRva;                         // 0 = Don't set to image
	IN     DWORD         dwImportTableRva;                         // 0 = Don't set to image
	IN     DWORD         dwResourceDirRva;                         // 0 = Don't set to image
	IN     DWORD         dwRelocationDirRva;                       // 0 = Don't set to image
	IN     DWORD         dwReserved01;                             // Reserved (do not use)
	IN     DWORD         dwReserved02;                             // Reserved (do not use)
	IN     DWORD         dwReserved03;                             // Reserved (do not use)
} PTDS_FULL_DUMP, *PPTDS_FULL_DUMP;

typedef struct _PTDS_PARTIAL_DUMP
{
	IN     DWORD         dwStructSize;                             // sizeof(PTDS_PARTIAL_DUMP)
	IN     DWORD         dwPID;
	IN     DWORD         dwBlockStartAddress;
	IN     DWORD         dwBlockSize;
	IN     BOOL          bSaveViaOfnDlg;
	OUT    BOOL          bDumpSuccessfully;
	OUT    BOOL          bUserSaved;                               // bSaveViaOfnDlg
	OUT    CHAR          cSavedTo[MAX_PATH];                       // bSaveViaOfnDlg
	OUT    LPVOID        pDumpedImage;                             // !bSaveViaOfnDlg
	IN     DWORD         dwReserved01;                             // Reserved (do not use)
	IN     DWORD         dwReserved02;                             // Reserved (do not use)
	IN     DWORD         dwReserved03;                             // Reserved (do not use)
} PTDS_PARTIAL_DUMP, *PPTDS_PARTIAL_DUMP;

#if defined(_MSC_VER)
#pragma pack()
#endif

#endif // __PTDS_h__