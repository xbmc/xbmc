CC=gcc
CFLAGS=-fPIC -D_LINUX
OBJS=png.o pngerror.o pngget.o pngmem.o pngpread.o pngread.o pngrio.o pngrtran.o pngrutil.o pngset.o pngtrans.o pngvcrd.o pngwio.o pngwrite.o pngwtran.o pngwutil.o

%o : %c
	$(CC) $(CFLAGS) -c -o $@ $< 

png.a: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
