
SUBDIRS=libaacs libbdplus libbdnav . examples


AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_builddir)/src/libbdplus

# The master library,
lib_LTLIBRARIES=libbluray.la

libbluray_la_SOURCES= bluray.c bluray.h
libbluray_la_LIBADD = libbdplus/libbdplus.la libbdnav/libbdnav.la
libbluray_la_LDFLAGS= -version-info 0:0:0


