## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_flux_SOURCES = $(COMMON) $(COLOR) $(VECTOR) flux.cc flux.hh trail.cc trail.hh
rs_flux_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS)

hack_PROGRAMS = rs-flux
