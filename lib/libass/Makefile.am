ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = libass.pc.in Changelog

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libass.pc

if HAVE_LIBPNG
    test = test
endif

SUBDIRS = libass $(test)

