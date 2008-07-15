CC=gcc
OBJS=libexif.o ExifParse.o IptcParse.o JpegParse.o
CFLAGS=-D_LINUX -D_DLL -fPIC -O2

libexif-i486-linux.so: $(OBJS)
	$(CC) -shared -o $@ *.o `cat ../../cores/DllLoader/exports/wrapper.def` ../../cores/DllLoader/exports/wrapper.o && cp $@ ../../../system/

clean:
	$(RM) $(OBJS) *.so

%o : %cpp
	g++ -c -o $@ $< $(CFLAGS)

%o : %c
	$(CC) -c -o $@ $< $(CFLAGS)
