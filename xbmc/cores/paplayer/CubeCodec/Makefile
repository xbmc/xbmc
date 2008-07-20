OBJS=adp.o adx.o afc.o cube.o decode.o dsp.o pcm.o vag.o XBMCCube.o
CFLAGS +=-D_LINUX -fPIC -O2
CXXFLAGS += _D_LINUX -fPIC -O2

LIB=../../../../system/cube-i486-linux.so

include ../../../../Makefile.include

lib: $(LIB)

$(LIB): $(OBJS)
	$(CC) -shared -o $@ *.o `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o
