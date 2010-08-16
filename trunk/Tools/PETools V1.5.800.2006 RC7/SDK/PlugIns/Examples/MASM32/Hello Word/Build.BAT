@echo off
\masm32\bin\ml /c /coff /Cp HelloWord.asm
\masm32\bin\Link /SECTION:.bss,S  /DLL /DEF:HelloWord.def /SUBSYSTEM:WINDOWS /LIBPATH:\masm32\lib HelloWord.obj
del HelloWord.obj
pause