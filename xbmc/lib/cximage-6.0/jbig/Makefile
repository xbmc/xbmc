CFLAGS+=-fPIC
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common
endif
LIB=jbig.a
OBJS=jbig.o jbig_tab.o 

include ../../../../Makefile.include

