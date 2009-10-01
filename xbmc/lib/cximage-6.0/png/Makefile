CFLAGS+=-fPIC
ifeq ($(findstring osx,$(ARCH)), osx)
	CFLAGS+=-fno-common
endif
LIB=png.a
OBJS=png.o pngerror.o pngget.o pngmem.o pngpread.o pngread.o pngrio.o pngrtran.o pngrutil.o pngset.o pngtrans.o pngvcrd.o pngwio.o pngwrite.o pngwtran.o pngwutil.o

include ../../../../Makefile.include

