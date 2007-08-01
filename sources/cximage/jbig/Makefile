CC=gcc
CFLAGS=-fPIC
OBJS=jbig.o jbig_tab.o tstcodec.o

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

jbig.a: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
