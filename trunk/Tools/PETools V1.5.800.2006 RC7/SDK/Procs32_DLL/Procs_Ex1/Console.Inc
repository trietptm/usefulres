.586
.MODEL flat, stdcall

OPTION CASEMAP:NONE   ;Case sensitive

Include windows.inc
Include kernel32.inc
Include masm32.inc

IncludeLib kernel32.lib
IncludeLib masm32.lib

Main		PROTO

;;macros
Print_Text Macro txt:REQ ;;note txt is not a pointer
	Invoke StdOut,ADDR txt
EndM

Get_Input Macro prompt:REQ,buffer:REQ ;;note prompt is not a pointer
	Invoke StdOut,ADDR prompt
	Invoke StdIn,ADDR buffer, LengthOf buffer
EndM


.DATA
Msg1	DB "Please Type Something: ",0AH,0DH,0
Msg2	DB "You Typed: ",0
Msg4	DB "Press Enter to Exit",0
CRLF	DB 0AH,0DH,0

.DATA?
inbuf		DB 100 DUP (?)
textbuf2	DB 100 DUP (?)
