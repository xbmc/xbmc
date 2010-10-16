OBJS = htsmsg.o \
       htsmsg_binary.o \
       htsbuf.o \
       htsstr.o \
       net_posix.o \
       sha1.o \

CFLAGS=-std=c99 -D_GNU_SOURCE -fPIC
LIB=libhts.a

# all is the default rule
all: $(LIB)

$(LIB): $(OBJS)
	rm -f $@
	ar cq $@ $(OBJS)

clean:
	-rm -f $(OBJS) $(LIB) *~
CLEAN: clean
