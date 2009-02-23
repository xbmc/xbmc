CC=gcc
CFLAGS+=-O2 -fPIC -D_LINUX

ifeq ($(ARCH), osx)
	CFLAGS+=-fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif

OBJS=png.o pngerror.o pngget.o pngmem.o pngpread.o pngread.o pngrio.o pngrtran.o pngrutil.o pngset.o pngtrans.o pngvcrd.o pngwio.o pngwrite.o pngwtran.o pngwutil.o

%o : %c
	$(CC) $(CFLAGS) -c -o $@ $< 

png.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) png.a
