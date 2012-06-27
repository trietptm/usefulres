
!ifdef __VC__
MAKEENV=makeenv_vc.mak
!else
MAKEENV=makeenv_br.mak
!endif

ALLDIRS=module ldr plugins

.PHONY: $(ALLDIRS) all env allbin

all: env allbin

MAKE_ALL=1
!include $(MAKEENV)

allbin: $(ALLDIRS)

!include alldirs.mk
