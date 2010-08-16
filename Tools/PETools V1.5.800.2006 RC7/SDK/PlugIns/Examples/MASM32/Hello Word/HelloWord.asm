;#####################################################
;
; PE Tools Sample PlugIn
;
; Coded by NEOx <NEOx@Pisem.net>
;
; Copyright [c] 2003, Underground InformatioN Center
;
; http://www.uinc.ru/
;
;#####################################################

.386
.model flat, stdcall
option casemap:none ; case sensitive

include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
include \masm32\include\windows.inc

includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\user32.lib

; ###[CONST]#####################################################################
.const
StartPTPlugin PROTO:DWORD
GetPTPluginName PROTO

; ###[DATA]######################################################################
.data
szDlgTitle    db "PE Tools PlugIn (in MASM32)", 0
szMsg         db "Hello Word!", 0


; ###[CODE]######################################################################
.code

; ###[ENTRY POINT]###############################################################
DllEntry proc hInstance:HINSTANCE, reason:DWORD, reserved1:DWORD
	mov  eax, TRUE
	ret
DllEntry Endp

; Activate PlugIn
StartPTPlugin proc hDlg:DWORD
      invoke MessageBox, hDlg, ADDR szMsg, ADDR szDlgTitle, MB_OK
      ret	
StartPTPlugin endp

; Get PlugIn Name
GetPTPluginName proc
      mov eax, offset szMsg
      ret
GetPTPluginName endp

End DllEntry
; ###[END]######################################################################