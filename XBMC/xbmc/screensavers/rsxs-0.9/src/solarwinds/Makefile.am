## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_solarwinds_SOURCES = $(COMMON) $(COLOR) $(VECTOR) solarwinds.cc \
	solarwinds.hh wind.cc wind.hh ../../../../../addons/xbmc_addon_lib++.a
rs_solarwinds_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS)

hack_PROGRAMS = rs-solarwinds
