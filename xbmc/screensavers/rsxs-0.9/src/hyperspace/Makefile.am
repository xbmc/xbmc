## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_hyperspace_SOURCES = $(COMMON) $(COLOR) $(VECTOR) $(IMPLICIT) $(IMAGE) \
	caustic.cc caustic.hh cubemaps.cc cubemaps.hh extensions.cc extensions.hh \
	flares.cc flares.hh goo.cc goo.hh hyperspace.cc hyperspace.hh nebula.cc \
	nebula.hh particle.cc particle.hh starburst.cc starburst.hh shaders.cc \
	shaders.hh spline.cc spline.hh tunnel.cc tunnel.hh

AM_CPPFLAGS += $(IMAGE_CPPFLAGS)
AM_CXXFLAGS += $(IMAGE_CXXFLAGS)
AM_LDFLAGS += $(IMAGE_LDFLAGS)
rs_hyperspace_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS)

hack_PROGRAMS = rs-hyperspace

hyperspacedatadir = $(pkgdatadir)/hyperspace
dist_hyperspacedata_DATA = nebula.png
