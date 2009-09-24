CFLAGS=-D_LINUX -fPIC -I../../include

dumb-i486-linux.so: DumbDLL.o
	g++ -shared -o $@ *.o ../../lib/unix/libdumb.a `cat ../../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

clean:
	$(RM) *.o *.so

DumbDLL.o : DumbDLL.cpp
	g++ -c -o $@ $< $(CFLAGS)
