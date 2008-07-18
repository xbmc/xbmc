CC=gcc
CFLAGS=-fPIC -D_LINUX
OBJS=jbig.o jbig_tab.o 

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

jbig.a: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
