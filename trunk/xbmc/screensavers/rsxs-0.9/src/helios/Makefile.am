## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_helios_SOURCES = $(COMMON) $(COLOR) $(VECTOR) $(IMAGE) $(IMPLICIT) \
	helios.cc helios.hh particle.cc particle.hh sphere.hh

AM_CPPFLAGS += $(IMAGE_CPPFLAGS)
AM_CXXFLAGS += $(IMAGE_CXXFLAGS)
AM_LDFLAGS += $(IMAGE_LDFLAGS)
rs_helios_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS)

hack_PROGRAMS = rs-helios

heliosdatadir = $(pkgdatadir)/helios
dist_heliosdata_DATA = spheremap.png

uninstall-hook:
	-rmdir $(heliosdatadir)
