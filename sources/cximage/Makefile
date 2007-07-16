DIRS=CxImage j2k jbig png tiff jasper jpeg 

ImageLib.so: compile
	gcc -shared -o ImageLib-i486-linux.so CxImage/*.o j2k/*.o jbig/*.o png/*.o tiff/*.o jasper/*/*.o jpeg/*.o /usr/lib/libz.a `cat ../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o

compile:
	$(MAKE) -C CxImage
	$(MAKE) -C j2k
	$(MAKE) -C jbig
	$(MAKE) -C png
	$(MAKE) -C tiff
	$(MAKE) -C jasper
	$(MAKE) -C jpeg

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done
	$(RM) *.so
