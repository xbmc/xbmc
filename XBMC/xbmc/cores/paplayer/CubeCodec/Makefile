CC=gcc
OBJS=adp.o adx.o afc.o cube.o decode.o dsp.o pcm.o vag.o XBMCCube.o
CFLAGS=-D_LINUX -fPIC -O2

cube-i486-linux.so: $(OBJS)
	$(CC) -shared -o $@ *.o `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o && cp $@ ../../../../system/players/paplayer/

clean:
	$(RM) *.o *.so

%o : %cpp
	g++ -c -o $@ $< $(CFLAGS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)
