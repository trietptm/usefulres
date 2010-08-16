;###############################################################################
; Procs32.inc
;
; Version 1.5.70.2005
;
; Coded by NEOx <neox@pisem.net>
;
; Copyright [c] 2005, Underground InformatioN Center
;
; http://www.uinc.ru
;###############################################################################

IFNDEF _PROCS32_
_PROCS32_ EQU 1

; Structs
PROCESS_ENTRY STRUC
	lpFileName 			CHAR MAX_PATH DUP (?)
	dwPID 				DWORD 	?
	bSystemProcess 		BOOL 	?
PROCESS_ENTRY ENDS
PPROCESS_ENTRY 			TYPEDEF PTR PROCESS_ENTRY

MODULE_ENTRY STRUCT
	lpFileName 			CHAR MAX_PATH DUP (?)
	dwImageBase 		DWORD 	?
	dwImageSize 		DWORD 	?
	bSystemProcess 		BOOL 	?
MODULE_ENTRY ENDS
PMODULE_ENTRY 			TYPEDEF PTR MODULE_ENTRY

; Protos
GetProcessFirst 		PROTO :PPROCESS_ENTRY
GetProcessNext 			PROTO :PPROCESS_ENTRY
GetModuleFirst 			PROTO :PMODULE_ENTRY
GetModuleNext 			PROTO :PMODULE_ENTRY
GetNumberOfProcesses 	PROTO 
GetNumberOfModules 		PROTO :DWORD
GetProcessPath 			PROTO :DWORD, :LPSTR, :DWORD
GetProcessBaseSize 		PROTO :DWORD, :LPDWORD, :LPDWORD
GetModuleHandleEx 		PROTO :DWORD, :LPSTR
GetProcessPathID 		PROTO :LPSTR
IsProcessRunned 		PROTO :DWORD
GetProcessIDList 		PROTO :LPDWORD, :DWORD
GetModuleHandleList 	PROTO :DWORD, :LPDWORD, :DWORD
GetModuleImageSize 		PROTO :DWORD, :DWORD, :LPDWORD

ENDIF ;_PROCS32_