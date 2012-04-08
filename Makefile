unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<

HOSTCC       = gcc
HOSTCFLAGS   = -Wall -O2 -fomit-frame-pointer



all:	bjdiag.c
	$(HOSTCC)  bjdiag.c $(HOSTCFLAGS)  -o bjdiag -lpthread

clean:
	rm bjdiag

help:
	@echo  'Cleaning targets:'
	@echo  '  clean           - Remove compiled binary'
	@echo  'Compile target:  '
	@echo  '  all		  - Make all'
