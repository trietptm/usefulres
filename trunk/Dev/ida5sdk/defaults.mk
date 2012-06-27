!ifdef NDEBUG
CFGSUF=opt
OBJDIRSUF=opt
LIBDIRSUF=opt
!endif

#--------------------------- Main path'es end variables ---------------------
!ifndef BCDIR
BCDIR=c:\progra~1\borland\cbuild~1\  # BCC directory
!endif

!ifndef BC5_COM
BC5_COM=$(BCDIR)
!endif

!ifndef BC31
BC31=c:\bcc\                                            # Borland directory
!endif

!ifndef MSVC6DIR
MSVC6DIR=C:\PROGRA~1\miaf9d~1\vc98\	# msvc6
!endif

!ifndef MSVC8DIR
MSVC8DIR=c:\progra~1\micros~1\vc\  # for __AMD64__
!endif

!ifndef MSVCARMDIR
MSVCARMDIR=C:\Program Files\Microsoft eMbedded C++ 4.0\EVC\wce420\ # for __CEARM__
!endif

################################
# pdb/python/...

!ifndef MSVCDIR
MSVCDIR="c:\program files\microsoft visual studio\vc98"
!endif

################################
!ifndef ULINK
ULINK=$(IDA)\bin\ulink.exe
!endif
#LINK_ULINK = 1

_LNK_EXE_MAP=-m

PEUTIL=peutil.exe

################################
RS      =$(IDA)bin\                                     # host utilities directory

################################
RM=del                                          # File Remover
MKDIR=-@mkdir
CP=copy
MV=move

#for speed in recursion next line is strongly recommended
MAKE=$(MAKEDIR)\$(MAKE).exe

################################
COMPILE_EFD=1

################################

#NDEBUG=1