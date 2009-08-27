## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include .

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/share

noinst_LTLIBRARIES = libreplaygain_synthesis.la

libreplaygain_synthesis_la_SOURCES = replaygain_synthesis.c

EXTRA_DIST = \
	Makefile.lite \
	replaygain_synthesis_static.dsp \
	replaygain_synthesis_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
