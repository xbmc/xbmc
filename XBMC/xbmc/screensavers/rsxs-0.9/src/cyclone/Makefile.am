## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_cyclone_SOURCES = $(COMMON) $(COLOR) $(VECTOR) blend.cc blend.hh cyclone.cc \
	cyclone.hh particle.cc particle.hh ../../../../addons/IndependentHeaders/xbmc_addon_lib++.a
rs_cyclone_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS)

hack_PROGRAMS = rs-cyclone
