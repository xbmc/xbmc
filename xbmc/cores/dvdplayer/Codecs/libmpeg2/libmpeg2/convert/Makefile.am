AM_CFLAGS = $(OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

lib_LTLIBRARIES = libmpeg2convert.la
libmpeg2convert_la_SOURCES = rgb.c uyvy.c
libmpeg2convert_la_LIBADD = libmpeg2convertarch.la
libmpeg2convert_la_LDFLAGS = -no-undefined

noinst_LTLIBRARIES = libmpeg2convertarch.la
libmpeg2convertarch_la_SOURCES = rgb_mmx.c rgb_vis.c
libmpeg2convertarch_la_CFLAGS = $(OPT_CFLAGS) $(ARCH_OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpeg2convert.pc

EXTRA_DIST = convert_internal.h
