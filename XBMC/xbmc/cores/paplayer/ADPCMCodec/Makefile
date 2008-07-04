CC=gcc
OBJS=uXboxAdpcmDecoder.o ADPCMDll.o
CFLAGS=-D_LINUX -fPIC -O2

apdcm-i486-linux.so: $(OBJS)
	$(CC) -shared -o $@ *.o `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o && cp $@ ../../../../system/players/paplayer/

clean:
	$(RM) $(OBJS) *.so

%o : %cpp
	g++ -c -o $@ $< $(CFLAGS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)
