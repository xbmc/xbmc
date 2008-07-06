CFLAGS=-D_LINUX -fPIC -Iinclude -Ivc++
OBJS=libdts/downmix.o libdts/parse.o libdts/bitstream.o

dcacodec-i486-linux.so: $(OBJS)
	gcc -fPIC -shared -o $@ $(OBJS) `cat ../../DllLoader/exports/wrapper.def` ../../DllLoader/exports/wrapper.o && cp $@ ../../../../system/players/paplayer/

clean:
	$(RM) $(OBJS) *.so

%o : %c
	gcc $(CFLAGS) -O2 -fPIC -c $< -o $@

%o : %cpp
	g++ $(CFLAGS) -O2 -fPIC -c $< -o $@
