CFLAGS+=-fPIC -DXBMC
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common
endif
LIB=raw.a
OBJS=libdcr.o

include ../../../Makefile.include

