CFLAGS=-D_LINUX -fPIC -Iinclude -Ivc++
OBJS=libdts/downmix.o libdts/parse.o libdts/bitstream.o

dcacodec-i486-linux.so: $(OBJS)
	gcc -fPIC -shared -o $@ $(OBJS) `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) $(OBJS) *.so

%o : %c
	gcc $(CFLAGS) -O2 -fPIC -c $< -o $@

%o : %cpp
	g++ $(CFLAGS) -O2 -fPIC -c $< -o $@
