@echo off
\MASM32\BIN\Ml.exe /c /coff LOGTester.asm
\MASM32\BIN\Link.exe /SUBSYSTEM:WINDOWS /MERGE:.idata=.text /MERGE:.data=.text /MERGE:.rdata=.text /SECTION:.text,EWR /IGNORE:4078 LOGTester.OBJ
del LOGTester.obj
pause