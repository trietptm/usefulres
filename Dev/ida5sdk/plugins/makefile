!include ../allmake.mak

ALLDIRS=choose2 getlines funclist procext vcsample uunp pdb subwindow ugraph

!if !$d(__EA64__) && $d(__NT__)
ALLDIRS=$(ALLDIRS) chart
!endif


.PHONY: $(ALLDIRS)

all:    $(ALLDIRS)

!include ../alldirs.mk

$(R)plugins\plugins.cfg: plugins.cfg
        @if not exist "$(R)plugins$(CHKDIR)" mkdir $(R)plugins
        $(CP) $? $@
