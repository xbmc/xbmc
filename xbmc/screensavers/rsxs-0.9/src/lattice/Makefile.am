## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_lattice_SOURCES = $(COMMON) $(VECTOR) $(IMAGE) camera.hh lattice.cc \
	lattice.hh resources.cc resources.hh

AM_CPPFLAGS += $(IMAGE_CPPFLAGS)
AM_CXXFLAGS += $(IMAGE_CXXFLAGS)
AM_LDFLAGS += $(IMAGE_LDFLAGS)
rs_lattice_LDADD = $(LIBMISC) $(X_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS)

hack_PROGRAMS = rs-lattice

latticedatadir = $(pkgdatadir)/lattice
dist_latticedata_DATA = brass.png chrome.png circuits.png crystal.png \
	doughnuts.png ghostly.png industrial1.png industrial2.png shiny.png

uninstall-hook:
	-rmdir $(latticedatadir)
