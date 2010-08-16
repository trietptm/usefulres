//////////////////////////////////////////////////////////////////////////////
//
// Include file for PE Tools Dumper Server v0.7 (Delphi)
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

{$A4}

unit PTDS;
interface
uses Windows, Messages;

//
// Constants
//
const

PTDS_INTERFACE_VERSION              $00000007       // 0.7
PTDS_WND_NAME                       "PE Tools Dumper Server"

//
// Commands
//
WM_PTDS_CMD_FIRST                   WM_USER           + $6000

// wParam - Client PID
// lParam - PPTDS_INFO_LOG
WM_PTDS_CMD_ADDLOG                  WM_PTDS_CMD_FIRST + 10

// wParam - Client PID
// lParam - PPTDS_VERSION
WM_PTDS_CMD_QUERYVERSION            WM_PTDS_CMD_FIRST + 20

// wParam - Client PID
// lParam - PPTDS_ENUM_PIDS
WM_PTDS_CMD_ENUMPROCESSIDS          WM_PTDS_CMD_FIRST + 30

// wParam - Client PID
// lParam - PPTDS_ENUM_PROCESSMODULES
WM_PTDS_CMD_ENUMPROCESSMODULES      WM_PTDS_CMD_FIRST + 40

// wParam - Client PID
// lParam - PPTDS_MODULE_INFO
WM_PTDS_CMD_QUERYPROCESSMODULEINFO  WM_PTDS_CMD_FIRST + 50

// wParam - Client PID
// lParam - PPTDS_FIND_PID
WM_PTDS_CMD_FINDPROCESSID           WM_PTDS_CMD_FIRST + 60

// wParam - Client PID
// lParam - PPTDS_FULL_DUMP
WM_PTDS_CMD_DUMPPROCESSMODULE       WM_PTDS_CMD_FIRST + 70

// wParam - Client PID
// lParam - PPTDS_PARTIAL_DUMP
WM_PTDS_CMD_DUMPPROCESSBLOCK        WM_PTDS_CMD_FIRST + 80

//
// Dump options
//
PTDS_DUMP_CORRECTIMAGESIZE          $00000001
PTDS_DUMP_PASTEHEADERFROMDISK       $00000002
PTDS_DUMP_FORCERAWMODE              $00000004
PTDS_DUMP_SAVEVIAOFNDLG             $00000008

//
// Rebuilding options
//
PTDS_REB_REBUILDIMAGE               $00010000
PTDS_REB_WIPERELOCATION             $00020000
PTDS_REB_VALIDATEIMAGE              $00040000
PTDS_REB_CHANGEIMAGEBASE            $00080000
PTDS_REB_REBUILDIMPORTS             $00100000                            // Reserved (do not use)
PTDS_REB_REBUILDRESOURCE            $00200000                            // Reserved (do not use)

//
// Structures
//
type
        PPTDS_INFO_LOG              = ^PTDS_INFO_LOG;
        PTDS_INFO_LOG               = record;
            dwStructSize            : DWORD;                             // sizeof(PTDS_INFO_LOG)
            szStr                   : PChar;                             // String
            dwStrSize               : DWORD;                             // Terminated NULL-character
        End;

        PPTDS_VERSION               = ^PTDS_VERSION;
        PTDS_VERSION                = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_VERSION)
            dwVersion               : DWORD;
        End;

        PPTDS_ENUM_PIDS             = ^PTDS_ENUM_PIDS;
        PTDS_ENUM_PIDS              = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_ENUM_PIDS)
            dwChainSize             : DWORD;                             // Array size
            pdwPIDChain             : ^DWORD;                            // DWORD array for PIDs
	    dwItemCount             : DWORD;
        End;

        PPTDS_ENUM_PROCESS_MODULES  = ^PTDS_ENUM_PROCESS_MODULES;
        PTDS_ENUM_PROCESS_MODULES   = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_ENUM_PROCESS_MODULES)
            dwPID                   : DWORD;
            dwChainSize             : DWORD;                             // Array size
	    pdwImageBaseChain       : ^DWORD;                            // DWORD array for module handles
	    dwItemCount             : DWORD;
        End;

        PPTDS_MODULE_INFO           = ^PTDS_MODULE_INFO;
        PTDS_MODULE_INFO            = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_MODULE_INFO)
            dwPID                   : DWORD;
            hImageBase              : HModule;                           // if NULL - calling module is dumped
	    dwImageSize             : DWORD;
            cModulePath             : Array [0..MAX_PATH - 1] Of Char;
        End;

        PPTDS_FIND_PID              = ^PTDS_FIND_PID;
        PTDS_FIND_PID               = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_FIND_PID)
            cProcessPath            : Array [0..MAX_PATH - 1] Of Char;
	    dwPID                   : DWORD;
        End;

        PPTDS_FULL_DUMP             = ^PTDS_FULL_DUMP;
        PTDS_FULL_DUMP              = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_FULL_DUMP)
            dwFlags                 : DWORD;                             // PTDS_DUMP_XXX/PTDS_REB_XXX
	    dwPID                   : DWORD;
            hModuleBase             : HModule;                           // NULL - calling module is dumped
	    bDumpSuccessfully       : Boolean;
            dwSizeOfDumpedImage     : DWORD;
            bUserSaved              : Boolean;                           // PTDS_DUMP_SAVEVIAOFNDLG
	    cSavedTo                : Array [0..MAX_PATH - 1] Of Char;   // PTDS_DUMP_SAVEVIAOFNDLG
	    pDumpedImage            : Pointer;                           // !PTDS_DUMP_SAVEVIAOFNDLG
	    dwRealignType           : DWORD;
	    dwNewImageBase          : DWORD;
	    dwEntryPointRva         : DWORD;                             // 0 = Don't set to image
	    dwExportTableRva        : DWORD;                             // 0 = Don't set to image
	    dwImportTableRva        : DWORD;                             // 0 = Don't set to image
	    dwResourceDirRva        : DWORD;                             // 0 = Don't set to image
	    dwRelocationDirRva      : DWORD;                             // 0 = Don't set to image
	    dwReserved01            : DWORD;                             // Reserved (do not use)
	    dwReserved02            : DWORD;                             // Reserved (do not use)
	    dwReserved03            : DWORD;                             // Reserved (do not use)
        End;

        PPTDS_PARTIAL_DUMP          = ^PTDS_PARTIAL_DUMP;
        PTDS_PARTIAL_DUMP           = record
            dwStructSize            : DWORD;                             // sizeof(PTDS_PARTIAL_DUMP)
            dwPID                   : DWORD;
            dwBlockStartAddress     : DWORD;
	    dwBlockSize             : DWORD;
            bSaveViaOfnDlg          : Boolean;
            bDumpSuccessfully       : Boolean;
            bUserSaved              : Boolean;                           // bSaveViaOfnDlg
	    cSavedTo                : Array [0..MAX_PATH-1] Of Char;     // bSaveViaOfnDlg
	    pDumpedImage            : Pointer;                           // !bSaveViaOfnDlg
	    dwReserved01            : DWORD;                             // Reserved (do not use)
	    dwReserved02            : DWORD;                             // Reserved (do not use)
	    dwReserved03            : DWORD;                             // Reserved (do not use)
        End;

implementation
end.