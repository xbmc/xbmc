CC=gcc
CFLAGS=-fPIC -DXBMC
OBJS=dcr.o libdcr.o

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

raw.a: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
