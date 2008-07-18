CFLAGS=-fPIC -DXBMC -D_LINUX
OBJS=dcr.o libdcr.o

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

raw.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) raw.a

include ../Makefile.include
