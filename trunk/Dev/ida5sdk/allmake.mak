#
#       Common part of make files for IDA.
#
#       Please set IDA and BCDIR variables!
#

#       All makesfiles are prepared to be used by BCC MAKE.EXE

#--------------------------- Main IDA directory   --------------------------
!ifndef IDA
IDA=z:\idasrc\current\  # with the trailing backslash!
!endif

!ifdef __AMD64__
__EA64__=1
!endif

!include $(IDA)defaults.mk

!ifndef IDALIBS
IDALIBS=$(IDA)
!endif

######################### set TV variables
!ifndef TVSRC
TVSRC=$(IDA)ui\txt\tv\	# TurboVision subdirectory
!endif

!ifndef IDA_TVL
IDA_TVL=$(TVSRC)
!endif

!ifdef _CFLAGS_TV	# set tv path(s) for ui/txt after include defaults.mk
_CFLAGS=-I$(TVSRC)include $(_CFLAGS_TV)
!endif

############################################################################
.PHONY: all All goal Goal total Total objdir test runtest $(ADDITIONAL_GOALS)

######################### set debug switches
!ifndef NDEBUG
CCDEBUG=-v
CCOPT=-O-
!else
CCOPT=-O2
!endif

!ifdef __EA64__
SUFF64=64
ADRSIZE=64
ADRSWITCH=-D__EA64__
!else
ADRSIZE=32
!endif

# include,help and other directories are common for all platforms and compilers:
I =$(IDA)include\       # include directory
R =$(IDA)bin\		# main result directory result
HO=$(R)                 # help file is stored in the bin directory
HI=$(RS)		# _ida.hlp placed in main tool directory
C =$(R)cfg\		# cfg files stored path
RI=$(R)idc\		# idc files stored path
HS=.hls                 #       help source
HH=.hhp                 #       help headers

#############################################################################
!if $d(__MSDOS__)					# Borland C++ DOS 16
__IDA_MSDOS__=1
__DOS16__=1
IMPLIB	=$(BC31)bin\implib.exe				# implib executable name
ASM	=$(BC31)bin\tasm.exe				# assembler
!ifdef LDR_TRANSLATION
CC	=$(BCDIR)bin\bcc.exe				# C++ compiler
!else
CC	=$(BC31)bin\bcc.exe			        # C++ compiler
!endif
CFLAGS	=+$(IDA)dosbor.cfg$(CFGSUF) $(_CFLAGS) $(CCDEBUG)	# default compiler flags
AFLAGS	=/t/ml/z/m5$(_AFLAGS)				# default assembler flags
OUTSW	=-e						# outfile name switch for one-line linker
OBJSW	=-n						# object file directory switch
OUTDLL	=/Twd						# output is DLL switch
LINKER	=$(BC31)bin\tlink /m $(_LDFLAGS)		# indirect file linker
CCL	=$(RS)ld _b _l$(CC) $(CFLAGS) $(_LDFLAGS)	# one-line linker
LINKSYS_EXE=      					# target link system for executable
LINKSYS	=						# target link system
C_STARTUP=$(BC31)lib\c0l				# indirect linker: C startup file
C_LIB	=$(BC31)lib\cl					# indirect linker: default C library
OVRON	=-Yo						# 16bit: following files overlayed
OVROFF	=-Yo-						# 16bit: following files not overlayed
LIBDIR	=$(IDALIBS)lib 					# libraries directory
TL	=$(IDA_TVL)lib\       		        	# TVision libraries directory
OBJDIR	=bc3.$(OBJDIRSUF)           			# object files directory
B	=.exe						# exe file extension
BS	=.exe						# host utility extension
MAP	=.map						# map file extension
T	=.ar$(LIBDIRSUF)                        	# library flag file extension
IDP	=.idp						# IDP extension
LDR     =.ldd                                           # LDR extension
PLUGIN  =.pl1                                           # PLUGIN extension
DLLEXT  =$(B)
LINKOPTS_EXE=/ye+/yx+/c
LINKOPTS=$(OUTDLL) $(LINKOPTS_EXE)
O	=.obj						# object file extension
A	=.lib						# library file extension
IDPSTUB	=$(LIBDIR)\idpstub$(O)				# STUB file for IDPs
LDRSTUB =$(LIBDIR)\idpstub$(O) $(L)dlldata$(O)          # STUB file for LDRs
IDPSLIB =						# system libraries for IDPs
AR	=$(RS)ar$(BS) _a _e$(T) "_l$(BC31)bin\tlib /C/0/E" ruv	# librarian
APISW   =-sdos16
#############################################################################
!elif $d(__AMD64__)                                     # Visual Studio 8 for AMD64
BCDIR   =$(MSVC8DIR)					#
CC      =$(BCDIR)bin\cl                                 # C++ compiler
!ifndef NDEBUG
#CCDEBUG=/Zi  -- linker complains about bad lib in my case
CCDEBUG=
CCOPT=/Od
!else
CCOPT=/O2
!endif
CFLAGS  =@$(IDA)w$(ADRSIZE)vs.cfg $(ADRSWITCH) $(CCDEBUG) $(CCOPT) $(_CFLAGS) $(_CLOPT) # default compiler flags
OUTSW   =/Fe                                            # outfile name switch for one-line linker
OBJSW	=/Fo						# object file directory switch
!ifdef BASE
LDFLAGS =/BASE:$(BASE) $(_LDFLAGS)
!else
LDFLAGS =$(_LDFLAGS)
!endif
ULINK_COMPAT_OPT = -O- -P- -o- -Gh -Gh- -F -N           # -N for pulgins\debugger
!ifdef LINK_ULINK
_LINKER =$(ULINK) $(ULINK_COMPAT_OPT) $(_CLOPT)         # _COMPAT_OPT used in UI\TXT
!else
_LINKER =$(BCDIR)bin\link                               # indirect file linker
!endif
LINKER  = $(_LINKER) $(LDFLAGS) $(CCDEBUG)              # default link command
CCL     =$(RS)ld _v _l$(CC) $(CFLAGS) $(_LDFLAGS)       # one-line linker
LINKSYS_EXE=                                            # target link system for executable
LINKSYS =                                               # target link system
C_STARTUP=                                              # indirect linker: C startup file
C_IMP   =kernel32                                       # import library
C_LIB   =$(C_IMP) libcmt                                # indirect linker: default C library
OVRON   =                                               # 16bit: following files overlayed
OVROFF  =                                               # 16bit: following files not overlayed
LIBDIR  =$(IDALIBS)libvc.x$(ADRSIZE)$(LIBDIRSUF)        # libraries directory
TL      =$(IDA_TVL)libvc.x64$(LIBDIRSUF)\               # TVision libraries directory
OBJDIR  =vc.x$(ADRSIZE)$(OBJDIRSUF)                     # object files directory
B       =x64.exe                                        # exe file extension
BS      =.exe                                           # host utility extension
MAP     =.mpv                                           # map file extension
T       =.ax$(ADRSIZE)$(LIBDIRSUF)                      # library flag file extension
IDP     =.x$(ADRSIZE)                                   # IDP extension
DLLEXT  =64.dll
IDP     =64.x$(ADRSIZE)                                 # IDP extension
LDR     =64.x$(ADRSIZE)                                 # LDR extension
PLUGIN  =.x$(ADRSIZE)                                   # PLUGIN extension
LINKOPTS_EXE=/LIBPATH:$(BCDIR)lib
LINKOPTS=$(LINKOPTS_EXE)
O       =.obj                                           # object file extension
A       =.lib                                           # library file extension
!if  !$d(NORTL)
IDPSTUB =		                                # STUB file for IDPs
LDRSTUB =                                               # STUB file for LDRs
IDPSLIB =$(C_LIB)                                       # system libraries for IDPs
!else
IDPSTUB =$(LIBDIR)\modstart                             # STUB file for IDPs
LDRSTUB =$(LIBDIR)\modstart                             # STUB file for LDRs
IDPSLIB =$(C_IMP)                                       # system libraries for IDPs
!endif
AR      =$(RS)ar$(BS) _e$(T) _v _llib ruv	        # librarian
APISW   =-swin -i$(RS)ida.imp -n
# force C mode for .c files
!if $d(DONT_FORCE_CPP)
FORCEC=/TC
!endif
#############################################################################
!elif $d(__CEARM__)                                     # Visual C++ v4.0 for ARM 4.20
BCDIR   =$(MSVCARMDIR)
CC      ="$(BCDIR)bin\clarm.exe"                        # C++ compiler
CCOPT=/O2
CCDEBUG=
CFLAGS  =@$(IDA)cearm.cfg $(ADRSWITCH) $(CCDEBUG) $(CCOPT) $(_CFLAGS) $(_CLOPT) # default compiler flags
CFLAG_SUFFIX = /link /subsystem:windowsce
OUTSW   =/Fe                                            # outfile name switch for one-line linker
OBJSW	=/Fo						# object file directory switch
!ifdef BASE
LDFLAGS =/BASE:$(BASE) $(_LDFLAGS)
!else
LDFLAGS =$(_LDFLAGS)
!endif
_LINKER =$(BCDIR)bin\link                               # indirect file linker
LINKER  =$(_LINKER) $(LDFLAGS) $(CCDEBUG)               # default link command
CCL     =$(RS)ld _c _l$(CC) $(CFLAGS) $(_LDFLAGS)       # one-line linker
C_IMP   =coredll                                        # import library
C_LIB   =$(C_IMP)                                       # indirect linker: default C library
LIBDIR  =$(IDALIBS)libcearm.l32$(LIBDIRSUF)             # libraries directory
OBJDIR  =armc.e32$(OBJDIRSUF)                           # object files directory
B       =_arm.exe                                       # exe file extension
BS      =.exe                                           # host utility extension
MAP     =.mparm                                         # map file extension
T       =.acearm32$(LIBDIRSUF)                          # library flag file extension
IDP     =.cearm32                                       # IDP extension
DLLEXT  =.dll
IDP     =.cearm32                                       # IDP extension
LDR     =.cearm32                                       # LDR extension
PLUGIN  =.cearm32                                       # PLUGIN extension
LINKOPTS_EXE=/LIBPATH:$(BCDIR)lib
LINKOPTS=$(LINKOPTS_EXE)
O       =.obj                                           # object file extension
A       =.lib                                           # library file extension
IDPSLIB =$(C_LIB)                                       # system libraries for IDPs
AR      =$(RS)ar$(BS) _e$(T) _v _llib ruv	        # librarian
APISW   =-swin -i$(RS)ida.imp -n
# force C mode for .c files
!if $d(DONT_FORCE_CPP)
FORCEC=/TC
!endif
#############################################################################
!elif $d(__VC__)                                        # Visual Studio 6 for x86
BCDIR   =$(MSVC6DIR)
CC      =$(BCDIR)bin\cl                                 # C++ compiler
!ifndef NDEBUG
#CCDEBUG=/Zi
CCDEBUG=
CCOPT=/Od
!else
CCOPT=/O2
!endif
CFLAGS  =/nologo @$(IDA)w$(ADRSIZE)vs.cfg $(ADRSWITCH) $(CCDEBUG) $(CCOPT) $(_CFLAGS) $(_CLOPT) # default compiler flags
OUTSW   =/Fe                                            # outfile name switch for one-line linker
OBJSW	=/Fo						# object file directory switch
!ifdef BASE
LDFLAGS =/BASE:$(BASE) $(_LDFLAGS)
!else
LDFLAGS =$(_LDFLAGS)
!endif
ULINK_COMPAT_OPT = -O- -P- -o- -Gh -Gh- -F -N           # -N for pulgins\debugger
!ifdef LINK_ULINK
_LINKER =$(ULINK) $(ULINK_COMPAT_OPT) $(_CLOPT)         # _COMPAT_OPT used in UI\TXT
!else
_LINKER =$(BCDIR)bin\link                               # indirect file linker
!endif
LINKER  = $(_LINKER) $(LDFLAGS) $(CCDEBUG)              # default link command
CCL     =$(RS)ld _v _l$(CC) $(CFLAGS) $(_LDFLAGS)       # one-line linker
LINKSYS_EXE=                                            # target link system for executable
LINKSYS =                                               # target link system
C_STARTUP=xxx                                           # indirect linker: C startup file
C_IMP   =kernel32                                       # import library
C_LIB   =$(C_IMP) libcmt                                # indirect linker: default C library
OVRON   =                                               # 16bit: following files overlayed
OVROFF  =                                               # 16bit: following files not overlayed
LIBDIR  =$(IDALIBS)libvc.w$(ADRSIZE)$(LIBDIRSUF)        # libraries directory
TL      =$(IDA_TVL)libvc.x$(ADRSIZE)$(LIBDIRSUF)\       # TVision libraries directory
OBJDIR  =vc.w$(ADRSIZE)$(OBJDIRSUF)                     # object files directory
B       =v32.exe                                        # exe file extension
BS      =.exe                                           # host utility extension
MAP     =.mpv                                           # map file extension
T       =.avc$(ADRSIZE)$(LIBDIRSUF)                     # library flag file extension
IDP     =.vc$(ADRSIZE)                                  # IDP extension
DLLEXT  =vc32.dll
IDP     =_vc32.v$(ADRSIZE)                              # IDP extension
LDR     =_vc32.v$(ADRSIZE)                              # LDR extension
PLUGIN  =.x$(ADRSIZE)                                   # PLUGIN extension
LINKOPTS_EXE= $(_LNK_EXE_MAP) /LIBPATH:$(BCDIR)lib
LINKOPTS=$(LINKOPTS_EXE)
O       =.obj                                           # object file extension
A       =.lib                                           # library file extension
!if  !$d(NORTL)
IDPSTUB =$(BCDIR)lib\c0d32                              # STUB file for IDPs
LDRSTUB =$(BCDIR)lib\c0d32                              # STUB file for LDRs
IDPSLIB =$(C_LIB)                                       # system libraries for IDPs
!else
IDPSTUB =$(LIBDIR)\modstart                             # STUB file for IDPs
LDRSTUB =$(LIBDIR)\modstart                             # STUB file for LDRs
IDPSLIB =$(C_IMP)                                       # system libraries for IDPs
!endif
AR      =$(RS)ar$(BS) _e$(T) _v _llib ruv	        # librarian
APISW   =-swin -i$(RS)ida.imp -n
# force C mode for .c files
!if $d(DONT_FORCE_CPP)
FORCEC=/TC
!endif
#############################################################################
!elif $d(__NT__)                                        # Borland C++ for NT (WIN32)
IMPLIB  =$(BCDIR)bin\implib                             # implib executable name
ASM     =$(BC5_COM)bin\tasm32                           # assembler
CC      =$(BCDIR)bin\bcc32                              # C++ compiler
!ifdef __IDP__
CC_ALIGN= -ps                                           # Standard calling convention
!else
CC_ALIGN= -pr                                           # Register calling convention
!endif
!ifdef __PRECOMPILE__
CC_PRECOMPILE= -H
!endif
CFLAGS  =+$(IDA)w$(ADRSIZE)bor.cfg$(CFGSUF) $(ADRSWITCH) $(CC_PRECOMPILE) $(CC_ALIGN) $(CCDEBUG) $(CCOPT) $(_CFLAGS) $(_CLOPT) # default compiler flags
AFLAGS  =/D__FLAT__ /t/ml/m5$(_AFLAGS)                  # default assembler flags
OUTSW   =-n -e                                          # outfile name switch for one-line linker
OBJSW	=-n						# object file directory switch
OUTDLL  =/Tpd                                           # output is DLL switch
!ifdef BASE
NT_BSW  =-b=$(BASE)
!endif
LDFLAGS =$(NT_BSW) $(_LDFLAGS)
ULINK_COMPAT_OPT = -O- -P- -o- -Gh -Gh- -F -N           # -N for pulgins\debugger
!ifdef LINK_ULINK
_LINKER =$(ULINK) $(ULINK_COMPAT_OPT) $(_CLOPT)         # _COMPAT_OPT used in UI\TXT
!else
_LINKER =$(BCDIR)bin\ilink32 -Gn                        # indirect file linker
!endif
LINKER  = $(_LINKER) $(LDFLAGS) $(CCDEBUG)              # default link command
CCL     =$(RS)ld _b _l$(CC) $(CFLAGS) $(_LDFLAGS)       # one-line linker
LINKSYS_EXE=                                            # target link system for executable
LINKSYS =                                               # target link system
C_STARTUP=c0x32                                         # indirect linker: C startup file
C_IMP   =import32                                       # import library
C_LIB   =$(C_IMP) cw32mt                                # indirect linker: default C library
OVRON   =                                               # 16bit: following files overlayed
OVROFF  =                                               # 16bit: following files not overlayed
LIBDIR  =$(IDALIBS)libbor.w$(ADRSIZE)$(LIBDIRSUF)       # libraries directory
TL      =$(IDA_TVL)libbor.w32$(LIBDIRSUF)\              # TVision libraries directory
OBJDIR  =bor.w$(ADRSIZE)$(OBJDIRSUF)                    # object files directory
!ifdef __EA64__
B       =64.exe                                         # exe file extension
!else
B       =.exe                                           # exe file extension
!endif
BS      =.exe                                           # host utility extension
MAP     =.mpb                                           # map file extension
T       =.a$(ADRSIZE)$(LIBDIRSUF)                       # library flag file extension
IDP     =.w$(ADRSIZE)                                   # IDP extension
!ifdef __EA64__
DLLEXT  =64.wll
IDP     =64.w$(ADRSIZE)                                 # IDP extension
LDR     =64.l$(ADRSIZE)                                 # LDR extension
PLUGIN  =.p$(ADRSIZE)                                   # PLUGIN extension
!else
DLLEXT  =.wll
LDR     =.ldw
PLUGIN  =.plw
!endif
ORDINALS= #-o                                           # import functions by ordinals
# -c case sensitive
# -C clear state before linking
# -s detailed map of segments
# -m detailed map of publics
# -r verbose
LINKOPTS_EXE= $(_LNK_EXE_MAP) -c -C $(ORDINALS) $(CCDEBUG) -L$(BCDIR)lib
LINKOPTS=$(OUTDLL) $(LINKOPTS_EXE)
O       =.obj                                           # object file extension
A       =.lib                                           # library file extension
!if  !$d(NORTL)
IDPSTUB =$(BCDIR)lib\c0d32                              # STUB file for IDPs
LDRSTUB =$(BCDIR)lib\c0d32                              # STUB file for LDRs
IDPSLIB =$(C_LIB)                                       # system libraries for IDPs
!else
IDPSTUB =$(LIBDIR)\modstart                             # STUB file for IDPs
LDRSTUB =$(LIBDIR)\modstart                             # STUB file for LDRs
IDPSLIB =$(C_IMP)                                       # system libraries for IDPs
!endif
AR      =$(RS)ar$(BS) _a _e$(T) "_l$(BC5_COM)BIN\TLIB.EXE /C/E/P128" ruv   # librarian
APISW   =-swin -i$(RS)ida.imp -n
# force C mode for .c files
!if $d(DONT_FORCE_CPP)
FORCEC=-P-
!endif
#############################################################################
!else
!error          Unknown compiler/platform
!endif
#############################################################################

!if 0           # this is for makedep
F=
!else
F=$(OBJDIR)\                        # object files dir with backslash
L=$(LIBDIR)\                        # library files dir with backslash
!endif

HC=$(RS)ihc$(BS)                                # Help Compiler

IDALIB=$(L)ida$(A)
DUMB=$(L)dumb$(O)
HLIB=$(HI)_ida.hlp

########################################################################
!if   $d(__MSDOS__)
_MAKEFLAGS=-D__MSDOS__ -U__NT__
!elif $d(__NT__)
_MAKEFLAGS=-U__MSDOS__ -D__NT__
!else
_MAKEFLAGS=-U__MSDOS__ -U__NT__
!endif

!ifdef _MAKEOPTS
MAKEFLAGS=$(MAKEFLAGS) $(_MAKEFLAGS)
!else
MAKEFLAGS=$(_MAKEFLAGS)
!endif

!ifdef __EA64__
MAKEFLAGS=$(MAKEFLAGS) -D__EA64__
!endif

### for 'if exist DIRECTORY'
!IF "$(OS)" == "Windows_NT"
CHKDIR=
!ELSE
CHKDIR=/nul
!ENDIF

########################################################################
!ifndef CONLY
CONLY=-c
!endif

!ifdef __VC__
.cpp$(O):
        $(CC) $(CFLAGS) $(CONLY) /Fo$(F)${<:.cpp=.obj} $?
.c$(O):
        $(CC) $(CFLAGS) $(CONLY) $(FORCEC) /Fo$(F)${<:.c=.obj} $?
!else
.cpp$(O):
        $(CC) $(CFLAGS) $(CONLY) {$< }
.c$(O):
        $(CC) $(CFLAGS) $(CONLY) $(FORCEC) {$< }
.asm$(O):
        $(ASM) $(AFLAGS) $*,$(F)$*
!endif


.hls.hhp:
        $(HC) -t $(HLIB) -i$@ $?
########################################################################
