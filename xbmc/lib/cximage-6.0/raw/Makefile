CFLAGS+=-fPIC -DXBMC
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common
endif
LIB=raw.a
OBJS=dcr.o libdcr.o

include ../../../../Makefile.include

