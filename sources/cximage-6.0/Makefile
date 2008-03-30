DIRS=CxImage raw jbig tiff

ImageLib.so: compile
	gcc -bundle -flat_namespace -undefined suppress -shared -o ImageLib-osx.so CxImage/*.o jasper/*/*.o jbig/*.o jpeg/*.o png/*.o raw/*.o tiff/*.o

compile:
	$(MAKE) -C CxImage -fMakefile
	$(MAKE) -C jasper
	$(MAKE) -C jbig
	$(MAKE) -C raw
	$(MAKE) -C tiff -fMakefile.osx
	$(MAKE) -C jpeg
	$(MAKE) -C png

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done
	$(RM) *.so
