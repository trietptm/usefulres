$(ALLDIRS):
        cd $@
        @$(MAKE) $(MAKEFLAGS)
        @cd ..
