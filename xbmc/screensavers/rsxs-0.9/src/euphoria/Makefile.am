## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_euphoria_SOURCES = $(COMMON) $(COLOR) $(VECTOR) $(IMAGE) euphoria.cc \
	euphoria.hh wisp.cc wisp.hh

AM_CPPFLAGS += $(IMAGE_CPPFLAGS)
AM_CXXFLAGS += $(IMAGE_CXXFLAGS)
AM_LDFLAGS += $(IMAGE_LDFLAGS)
rs_euphoria_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS)

hack_PROGRAMS = rs-euphoria

euphoriadatadir = $(pkgdatadir)/euphoria
dist_euphoriadata_DATA = lines.png plasma.png stringy.png

uninstall-hook:
	-rmdir $(euphoriadatadir)
