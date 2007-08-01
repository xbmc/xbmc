CFLAGS=-D_LINUX -fPIC -DPACK -DUNPACK -DSEEKING -DTAGS -DVER3 -DUSE_FSTREAMS
OBJS=bits.o extra1.o extra2.o float.o md5.o metadata.o pack.o unpack.o unpack3.o utils.o words.o wputils.o

wavpack-i486-linux.so: $(OBJS)
	gcc -fPIC -shared -o $@ $(OBJS) `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) $(OBJS) *.so

%o : %c
	gcc $(CFLAGS) -O2 -fPIC -c $< -o $@

%o : %cpp
	g++ $(CFLAGS) -O2 -fPIC -c $< -o $@
