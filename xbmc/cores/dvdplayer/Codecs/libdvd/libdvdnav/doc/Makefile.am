include $(top_srcdir)/misc/Makefile.common

EXTRA_DIST = doxy.conf mainpage.cpp tutorial.cpp dvd_structures library_layout

docs: doxy.conf
	@if test x"$(DOXYGEN)" != "xno"; then \
	  $(DOXYGEN) doxy.conf > /dev/null 2> /dev/null; \
	fi
