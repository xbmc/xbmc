include ../../../Makefile.include

OBJS=libid3tag-@ARCH@.so

lib: libid3tag-@ARCH@.so
DIRS=libid3tag

libid3tag-@ARCH@.so:
	$(MAKE) -C libid3tag
	g++ -shared -o libid3tag-@ARCH@.so libid3tag/.libs/*.o `cat ../../cores/DllLoader/exports/wrapper.def` ../../cores/DllLoader/exports/wrapper.o -fPIC -O3 && cp $@ ../../../system/
