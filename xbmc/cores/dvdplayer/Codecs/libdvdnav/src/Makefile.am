include $(top_srcdir)/misc/Makefile.common

SUBDIRS = dvdread vm

includedir = ${prefix}/include/dvdnav

AM_CPPFLAGS = -DDVDNAV_COMPILE $(THREAD_CFLAGS) \
	-I$(top_srcdir)/src/dvdread -I$(top_srcdir)/src/vm

EXTRA_DIST = README.MAP FELLOWSHIP.map

lib_LTLIBRARIES = libdvdnav.la

libdvdnav_la_SOURCES = dvdnav.c \
	read_cache.c navigation.c highlight.c \
	searching.c settings.c remap.c \
	dvdnav_internal.h read_cache.h remap.h

libdvdnav_la_LIBADD = $(THREAD_LIBS) \
	$(top_builddir)/src/dvdread/libdvdread.la \
	$(top_builddir)/src/vm/libdvdvm.la

libdvdnav_la_LDFLAGS = \
	-version-info $(DVDNAV_LT_CURRENT):$(DVDNAV_LT_REVISION):$(DVDNAV_LT_AGE) \
	-export-symbols-regex "(^dvdnav.*|^nav.*|^ifo.*|^DVD.*)"
#	-release $(DVDNAV_MAJOR).$(DVDNAV_MINOR).$(DVDNAV_SUB)

include_HEADERS = dvdnav.h dvdnav_events.h dvd_types.h
