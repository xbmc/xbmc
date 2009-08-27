## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include/share

noinst_LTLIBRARIES = libutf8.la

libutf8_la_SOURCES = charset.c charset.h iconvert.c utf8.c

EXTRA_DIST = \
	Makefile.lite \
	charmaps.h \
	makemap.c \
	charset_test.c \
	charsetmap.h \
	iconvert.h \
	utf8_static.dsp \
	utf8_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
