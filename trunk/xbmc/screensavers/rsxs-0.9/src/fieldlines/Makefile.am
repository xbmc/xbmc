## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_fieldlines_SOURCES = $(COMMON) $(COLOR) $(VECTOR) fieldlines.cc \
	fieldlines.hh ion.hh
rs_fieldlines_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS)

hack_PROGRAMS = rs-fieldlines
