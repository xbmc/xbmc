EXTRA_DIST = doxygen.cfg.in footer.html header.html
DISTCLEANFILES = doxygen.cfg

all: stamp-doxygen stamp-latex

doxygen.cfg: $(srcdir)/doxygen.cfg.in
	-rm -f $@
	sed 's|@SRCDIR@|$(srcdir)|g; s|@TOP_SRCDIR@|$(top_srcdir)|g' $< > $@

stamp-doxygen: doxygen.cfg
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
	-rm -f stamp-latex stamp-doxygen
	-rm -Rf html latex

