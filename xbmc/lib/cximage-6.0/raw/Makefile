CFLAGS=-fPIC -DXBMC -D_LINUX
OBJS=dcr.o libdcr.o

raw.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(RM) $(OBJS) raw.a

distclean: clean

include ../Makefile.include
