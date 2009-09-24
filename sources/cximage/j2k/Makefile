CC=gcc
OBJS=bio.o cio.o dwt.o fix.o int.o j2k.o mct.o mqc.o pi.o t1.o t2.o tcd.o tgt.o
CFLAGS=-fPIC

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

j2k.a: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
