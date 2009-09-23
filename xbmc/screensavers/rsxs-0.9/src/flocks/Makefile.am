## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_flocks_SOURCES = $(COMMON) $(COLOR) $(VECTOR) bug.cc bug.hh flocks.cc \
	flocks.hh
rs_flocks_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS)

hack_PROGRAMS = rs-flocks
