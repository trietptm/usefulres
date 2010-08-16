;////////////////////////////////////////////////////////////////////////////
;
; Include file for PE Tools Dumper Server v0.7 (MASM/TASM)
; ----------------------------------------------------------------
;
; Version: 0.7
;
; Coded by NEOx <neox@pisem.net>
;
; Copyright [c] 2005, Underground InformatioN Center
;
; Visit us http://www.uinc.ru/
;////////////////////////////////////////////////////////////////////////////

;
; Constants
;
PTDS_INTERFACE_VERSION              EQU 00000007      ; 0.7

.CONST
PTDS_WND_NAME                       DB  "PE Tools Dumper Server", 0

;
; Commands
;
WM_PTDS_CMD_FIRST                   EQU WM_USER           + 6000h

; wParam - Client PID
; lParam - PPTDS_INFO_LOG
WM_PTDS_CMD_ADDLOG                  EQU WM_PTDS_CMD_FIRST + 10

; wParam - Client PID
; lParam - PPTDS_VERSION
WM_PTDS_CMD_QUERYVERSION            EQU WM_PTDS_CMD_FIRST + 20

; wParam - Client PID
; lParam - PPTDS_ENUM_PIDS
WM_PTDS_CMD_ENUMPROCESSIDS          EQU WM_PTDS_CMD_FIRST + 30

; wParam - Client PID
; lParam - PPTDS_ENUM_PROCESSMODULES
WM_PTDS_CMD_ENUMPROCESSMODULES      EQU WM_PTDS_CMD_FIRST + 40

; wParam - Client PID
; lParam - PPTDS_MODULE_INFO
WM_PTDS_CMD_QUERYPROCESSMODULEINFO  EQU WM_PTDS_CMD_FIRST + 50

; wParam - Client PID
; lParam - PPTDS_FIND_PID
WM_PTDS_CMD_FINDPROCESSID           EQU WM_PTDS_CMD_FIRST + 60

; wParam - Client PID
; lParam - PPTDS_FULL_DUMP
WM_PTDS_CMD_DUMPPROCESSMODULE       EQU WM_PTDS_CMD_FIRST + 70

; wParam - Client PID
; lParam - PPTDS_PARTIAL_DUMP
WM_PTDS_CMD_DUMPPROCESSBLOCK        EQU WM_PTDS_CMD_FIRST + 80


;
; Dump options
;
PTDS_DUMP_CORRECTIMAGESIZE          EQU 00000001h
PTDS_DUMP_PASTEHEADERFROMDISK       EQU 00000002h
PTDS_DUMP_FORCERAWMODE              EQU 00000004h
PTDS_DUMP_SAVEVIAOFNDLG             EQU 00000008h

;
; Rebuilding options
;
PTDS_REB_REBUILDIMAGE               EQU 00010000h
PTDS_REB_WIPERELOCATION             EQU 00020000h
PTDS_REB_VALIDATEIMAGE              EQU 00040000h
PTDS_REB_CHANGEIMAGEBASE            EQU 00080000h
;PTDS_REB_REBUILDIMPORTS             EQU 00100000h           ; Reserved (do not use)
;PTDS_REB_REBUILDRESOURCE            EQU 00200000h           ; Reserved (do not use)

;
; Structures
;

PTDS_INFO_LOG STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_INFO_LOG)
	szStr               LPSTR  ?                         ; String
	dwStrSize           DWORD  ?                         ; Terminated NULL-character
PTDS_INFO_LOG ENDS
PPTDS_INFO_LOG TYPEDEF PTR PTDS_INFO_LOG

PTDS_VERSION STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_VERSION)
	dwVersion           DWORD  ?
PTDS_VERSION ENDS
PPTDS_VERSION TYPEDEF PTR PTDS_VERSION

PTDS_ENUM_PIDS STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_ENUM_PIDS)
	dwChainSize         DWORD  ?                         ; Array size
	pdwPIDChain         LPVOID ?                         ; DWORD array for PIDs
	dwItemCount         DWORD  ?
PTDS_ENUM_PIDS ENDS
PPTDS_ENUM_PIDS TYPEDEF PTR PTDS_ENUM_PIDS

PTDS_ENUM_PROCESS_MODULES STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_ENUM_PROCESS_MODULES)
	dwPID               DWORD  ?
	dwChainSize         DWORD  ?                         ; Array size
	pdwImageBaseChain   LPVOID ?                         ; DWORD array for module handles
	dwItemCount         DWORD  ?
PTDS_ENUM_PROCESS_MODULES ENDS
PPTDS_ENUM_PROCESS_MODULES TYPEDEF PTR PTDS_ENUM_PROCESS_MODULES

PTDS_MODULE_INFO STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_MODULE_INFO)
	dwPID               DWORD  ?
	hImageBase          HANDLE ?                         ; if NULL - calling module is dumped
	dwImageSize         DWORD  ?
	cModulePath         CHAR MAX_PATH DUP (?)
PTDS_MODULE_INFO ENDS
PPTDS_MODULE_INFO TYPEDEF PTR PTDS_MODULE_INFO

PTDS_FIND_PID STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_FIND_PID)
	cProcessPath        CHAR MAX_PATH DUP (?)
	dwPID               DWORD  ?
PTDS_FIND_PID ENDS
PPTDS_FIND_PID TYPEDEF PTR PTDS_FIND_PID

PTDS_FULL_DUMP STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_FULL_DUMP)
	dwFlags             DWORD  ?                         ; PTDS_DUMP_XXX/PTDS_REB_XXX
	dwPID               DWORD  ?
	hModuleBase         HANDLE ?                         ; NULL - calling module is dumped
	bDumpSuccessfully   BOOL   ?
	dwSizeOfDumpedImage DWORD  ?
	bUserSaved          BOOL   ?                         ; PTDS_DUMP_SAVEVIAOFNDLG
	cSavedTo            CHAR MAX_PATH DUP (?)            ; PTDS_DUMP_SAVEVIAOFNDLG
	pDumpedImage        LPVOID ?                         ; !PTDS_DUMP_SAVEVIAOFNDLG
	dwNewImageBase      DWORD  ?                         ; format: 0xXXXX0000
	dwEntryPointRva     DWORD  ?                         ; 0 = Don't set to image
	dwExportTableRva    DWORD  ?                         ; 0 = Don't set to image
	dwImportTableRva    DWORD  ?                         ; 0 = Don't set to image
	dwResourceDirRva    DWORD  ?                         ; 0 = Don't set to image
	dwRelocationDirRva  DWORD  ?                         ; 0 = Don't set to image
	dwReserved01        DWORD  ?                         ; Reserved (do not use)
	dwReserved02        DWORD  ?                         ; Reserved (do not use)
	dwReserved03        DWORD  ?                         ; Reserved (do not use)
PTDS_FULL_DUMP ENDS
PPTDS_FULL_DUMP TYPEDEF PTR PTDS_FULL_DUMP

PTDS_PARTIAL_DUMP STRUCT
	dwStructSize        DWORD  ?                         ; sizeof(PTDS_PARTIAL_DUMP)
	dwPID               DWORD  ?
	dwBlockStartAddress DWORD  ?
	dwBlockSize         DWORD  ?
	bSaveViaOfnDlg      BOOL   ?
	bDumpSuccessfully   BOOL   ?
	bUserSaved          BOOL   ?                         ; bSaveViaOfnDlg
	cSavedTo            CHAR MAX_PATH DUP (?)            ; bSaveViaOfnDlg
	pDumpedImage        LPVOID ?                         ; !bSaveViaOfnDlg
	dwReserved01        DWORD  ?                         ; Reserved (do not use)
	dwReserved02        DWORD  ?                         ; Reserved (do not use)
	dwReserved03        DWORD  ?                         ; Reserved (do not use)
PTDS_PARTIAL_DUMP ENDS
PPTDS_PARTIAL_DUMP TYPEDEF PTR PTDS_PARTIAL_DUMP