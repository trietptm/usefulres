; ##############################################################################
;
; PE Tools Dumper Server - PlugIn
;
; Coded by NEOx <NEOx@Pisem.net>
;
; Copyright [c] 2004, Underground InformatioN Center
;
; http://www.uinc.ru/
;
; ##############################################################################

.386
.model flat, stdcall
option casemap :none ; case sensitive

include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\include\comdlg32.inc
include \masm32\include\shell32.inc
include \masm32\include\windows.inc

; PE Tools Dumper Server include file
include PTDS.inc

includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\user32.lib
includelib \masm32\lib\comdlg32.lib
includelib \masm32\lib\shell32.lib


PTDSLog PROTO hWnd :DWORD, szString :LPSTR, dwStringSize :DWORD

; ###[DATA]######################################################################
.data
szCaption                      DB "PE Tools PlugIn", 0
szRunPTDS                      DB "Launch 'PE Tools Dumper Server' first !", 0
szPlugInName                   DB "[LOG Tester] - PlugIn example for ASM by NEOx...", 0

dwMyPID                        DWORD  ?
hWndPTDS                       HANDLE ?
PTInfoLog                      PTDS_INFO_LOG <?>


; ###[CODE]######################################################################
.code

PTDSLog PROC hWnd :DWORD, szString :LPSTR, dwStringSize :DWORD
	MOV    PTInfoLog.dwStructSize, sizeof PTInfoLog
	MOV    EAX, szString
	MOV    PTInfoLog.szStr, EAX
	MOV    EAX, dwStringSize
	MOV    PTInfoLog.dwStrSize, EAX
	invoke SendMessage, hWnd, WM_PTDS_CMD_ADDLOG, dwMyPID, OFFSET PTInfoLog
	RET
PTDSLog ENDP

; ###[ENTRY POINT]###############################################################
start:
	; -> Get PTDS Window Handle
	invoke FindWindow, NULL, OFFSET PTDS_WND_NAME
	TEST   EAX, EAX
	JNZ    PSTDFounded
	invoke MessageBox, 0, OFFSET szRunPTDS, OFFSET szCaption, MB_OK or MB_ICONERROR
	JNZ    Exit
	
PSTDFounded:
	MOV    hWndPTDS, eax
	CALL   GetCurrentProcessId
	PUSH   EAX
	POP    dwMyPID ; -> Save my PID
	invoke PTDSLog, hWndPTDS, OFFSET szPlugInName, sizeof szPlugInName
	JNZ    Exit 
	
Exit:
	invoke  ExitProcess, 0
end start
; ###[END]######################################################################