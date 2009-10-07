INCLUDES=-I.
CC=gcc
CFLAGS+=-O2 -fPIC -D_LINUX 

OBJS = debug.o \
			 alloc.o \

%o : %c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

librefmem.a: $(OBJS)
	$(AR) rc $@ $(OBJS)

clean:
	$(RM) $(OBJS) librefmem.a
