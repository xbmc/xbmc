DIRS=CxImage j2k jbig tiff

ImageLib.so: compile	
	gcc -shared -o ImageLib-i486-linux.so					\
		CxImage/*.o							\
		j2k/*.o								\
		jbig/*.o							\
		-lpng								\
		tiff/*.o							\
		-ljasper							\
		-ljpeg								\
		/usr/lib/libz.a							\
		`cat ../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def`	\
		../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

compile:
	$(MAKE) -C CxImage
	$(MAKE) -C j2k
	$(MAKE) -C jbig
	$(MAKE) -C tiff

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done
	$(RM) *.so
