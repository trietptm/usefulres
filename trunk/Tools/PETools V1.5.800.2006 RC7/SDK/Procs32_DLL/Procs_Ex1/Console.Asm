Include Console.Inc
Include Procs32.Inc

includelib Procs32.lib

.CODE

Start:

  Invoke Main
  Invoke ExitProcess,0

Main Proc
	XOR EAX,EAX
	Get_Input Msg1, inbuf
	call GetNumberOfProcesses
	Print_Text Msg2
	Print_Text inbuf
	Print_Text textbuf2
	Print_Text CRLF
	Get_Input Msg4,inbuf
	RET
Main EndP

End Start
