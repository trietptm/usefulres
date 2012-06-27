_CFLAGS=$(__CFLAGS) -D__IDP__

!ifndef BASE
BASE=0x14000000
!endif

__IDP__=1
!ifndef O
!include ../../allmake.mak
!endif

!ifdef O1
OBJ1=$(F)$(O1)$(O)
!endif

!ifdef O2
OBJ2=$(F)$(O2)$(O)
!endif

!ifdef O3
OBJ3=$(F)$(O3)$(O)
!endif

!ifdef O4
OBJ4=$(F)$(O4)$(O)
!endif

!ifdef O5
OBJ5=$(F)$(O5)$(O)
!endif

!ifdef O6
OBJ6=$(F)$(O6)$(O)
!endif

!ifdef H1
HELPS=$(H1)$(HH)
!endif

OBJS=$(F)$(PROC)$(O) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) \
     $(OBJ7) $(_OBJS)

LDR_MODULE=$(R)loaders\$(PROC)$(LDR)

all:	objdir $(HELPS) $(LDR_MODULE) $(ADDITIONAL_GOALS)

$(LDR_MODULE): ..\ldr.def $(OBJS) $(IDALIB) makefile
!ifdef __AMD64__
	link $(LINKOPTS) /DLL /OUT:$@ /EXPORT:LDSC $(OBJS) $(IDALIB) user32.lib
!else
	$(LINKER) @&&~
$(LINKOPTS) $(LDRSTUB) $(OBJS)
$@

$(IDALIB) $(IDPSLIB)
..\ldr.def
~
        $(PEUTIL) -d..\ldr.def $@
!endif

!include ../../objdir.mak
