CC=gcc
OBJS=adp.o adx.o afc.o cube.o decode.o dsp.o pcm.o vag.o XBMCCube.o
CFLAGS=-D_LINUX -fPIC -O2

cube-i486-linux.so: $(OBJS)
	$(CC) -shared -o $@ *.o `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) *.o *.so

%o : %cpp
	g++ -c -o $@ $< $(CFLAGS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)
