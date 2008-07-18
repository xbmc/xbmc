CFLAGS=-fPIC -D_LINUX
OBJS=jbig.o jbig_tab.o 

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

jbig.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) jbig.a

distclean: clean

include ../Makefile.include
