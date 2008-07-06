CC=gcc
OBJS=alac.o demux.o stream.o
CFLAGS=-fPIC

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)

alac_decoder: $(OBJS)

clean:
	$(RM) $(OBJS)

include ../Makefile.include
