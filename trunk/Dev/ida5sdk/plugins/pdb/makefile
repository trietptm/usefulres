PROC=pdb
ADDITIONAL_GOALS=cfg
!include ..\plugin.mak

cfg: $(C)pdb.cfg
$(C)pdb.cfg: pdb.cfg
        $(CP) $? $@

# MAKEDEP dependency list ------------------
$(F)pdb$(O)     : $(I)area.hpp $(I)bytes.hpp $(I)demangle.hpp               \
	          $(I)diskio.hpp $(I)err.h $(I)fpro.h $(I)funcs.hpp         \
	          $(I)help.h $(I)ida.hpp $(I)idp.hpp $(I)kernwin.hpp        \
	          $(I)lines.hpp $(I)llong.hpp $(I)loader.hpp $(I)nalt.hpp   \
	          $(I)name.hpp $(I)netnode.hpp $(I)pro.h $(I)segment.hpp    \
	          $(I)ua.hpp $(I)xref.hpp pdb.cpp
