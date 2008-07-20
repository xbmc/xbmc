CFLAGS +=-D_LINUX -fPIC -I../../include
OBJS=DumbDLL.o

LIB=../../../../../../system/players/paplayer/dumb-i486-linux.so

include ../../../../../../Makefile.include

lib: $(LIB)

$(LIB): $(OBJS)
	g++ -shared -o $@ *.o ../../lib/unix/libdumb.a `cat ../../../../DllLoader/exports/wrapper.def` ../../../../DllLoader/exports/wrapper.o
