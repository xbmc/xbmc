
libid3tag-i486-linux.so:
	(cd libid3tag; CFLAGS=-O3 ./configure)
	$(MAKE) --directory=libid3tag
	(cd libid3tag; ./make-xbmc-lib.sh)
	mv libid3tag/$@ $@

clean:
	$(RM) libid3tag-i486-linux.so
	$(MAKE) --directory=libid3tag clean
