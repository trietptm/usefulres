PROC=dos
O1=dos_ovr

!ifdef __NT__
!ifndef __VC__
OBJ2=$(L)setjmp$(O)
!endif
!endif

!include ..\loader.mak

# MAKEDEP dependency list ------------------
$(F)dos$(O)     : $(I)area.hpp $(I)auto.hpp $(I)bytes.hpp $(I)diskio.hpp    \
	          $(I)entry.hpp $(I)exehdr.h $(I)fixup.hpp $(I)fpro.h       \
	          $(I)funcs.hpp $(I)help.h $(I)ida.hpp $(I)idp.hpp          \
	          $(I)kernwin.hpp $(I)lines.hpp $(I)llong.hpp               \
	          $(I)loader.hpp $(I)nalt.hpp $(I)name.hpp $(I)netnode.hpp  \
	          $(I)offset.hpp $(I)pro.h $(I)segment.hpp $(I)srarea.hpp   \
	          $(I)ua.hpp $(I)xref.hpp ../idaldr.h dos.cpp dos_ovr.h
$(F)dos_ovr$(O) : $(I)area.hpp $(I)auto.hpp $(I)bytes.hpp $(I)diskio.hpp    \
	          $(I)entry.hpp $(I)exehdr.h $(I)fixup.hpp $(I)fpro.h       \
	          $(I)funcs.hpp $(I)help.h $(I)ida.hpp $(I)idp.hpp          \
	          $(I)kernwin.hpp $(I)lines.hpp $(I)llong.hpp               \
	          $(I)loader.hpp $(I)nalt.hpp $(I)name.hpp $(I)netnode.hpp  \
	          $(I)offset.hpp $(I)pro.h $(I)segment.hpp $(I)srarea.hpp   \
	          $(I)struct.hpp $(I)ua.hpp $(I)xref.hpp ../idaldr.h        \
	          dos_ovr.cpp dos_ovr.h
