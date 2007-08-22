EXTRA_DIST = doxygen.cfg footer.html header.html

all: stamp-doxygen stamp-latex

stamp-doxygen:
if DOXYGEN
	doxygen doxygen.cfg
	touch stamp-doxygen
endif

stamp-latex: stamp-doxygen
if DOXYGEN
if LATEX
	cd latex && $(MAKE) $(AM_CFLAGS) ps
	touch stamp-latex
endif
endif

clean: clean-local
clean-local:
	-rm -Rf html latex
	-rm -f stamp-latex stamp-doxygen

