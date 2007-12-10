## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libgrabbag.la

libgrabbag_la_SOURCES = \
	cuesheet.c \
	file.c \
	picture.c \
	replaygain.c \
	seektable.c

EXTRA_DIST = \
	Makefile.lite \
	grabbag_static.dsp \
	grabbag_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
