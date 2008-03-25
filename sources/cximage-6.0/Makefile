DIRS=CxImage jasper jbig jpeg png raw tiff

ImageLib-osx.so: compile
	gcc -bundle -flat_namespace -undefined suppress -shared -o ImageLib-osx.so CxImage/*.o jasper/*/*.o jbig/*.o jpeg/*.o png/*.o raw/*.o tiff/*.o

compile:
	$(MAKE) -C CxImage
	$(MAKE) -C jasper
	$(MAKE) -C jbig
	$(MAKE) -C jpeg
	$(MAKE) -C png
	$(MAKE) -C raw
	$(MAKE) -C tiff

clean:
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done
	$(RM) *.so
