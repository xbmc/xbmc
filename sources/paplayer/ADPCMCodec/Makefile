CC=gcc
OBJS=uXboxAdpcmDecoder.o ADPCMDll.o
CFLAGS=-D_LINUX -fPIC -O2

apdcm-i486-linux.so: $(OBJS)
	$(CC) -shared -o $@ *.o `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) $(OBJS) *.so

%o : %cpp
	g++ -c -o $@ $< $(CFLAGS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)
