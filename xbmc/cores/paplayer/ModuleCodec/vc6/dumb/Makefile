CFLAGS=-D_LINUX -fPIC -I../../include

dumb-i486-linux.so: DumbDLL.o
	g++ -shared -o $@ *.o ../../lib/unix/libdumb.a `cat ../../../../DllLoader/exports/wrapper.def` ../../../../DllLoader/exports/wrapper.o && cp $@ ../../../../../../system/players/paplayer/

clean:
	$(RM) *.o *.so

DumbDLL.o : DumbDLL.cpp
	g++ -c -o $@ $< $(CFLAGS)
