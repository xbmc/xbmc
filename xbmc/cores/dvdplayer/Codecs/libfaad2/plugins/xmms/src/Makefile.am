local_CFLAGS=`$(XMMS_CONFIG) --cflags` -Wall
local_LDFLAGS=`$(XMMS_CONFIG) --libs`
libdir = `$(XMMS_CONFIG) --input-plugin-dir`
lib_LTLIBRARIES = libmp4.la

libmp4_la_CFLAGS = $(local_CFLAGS) -Wall \
       -I$(top_srcdir)/include -I$(top_srcdir)/common/mp4ff

libmp4_la_LIBADD = $(top_builddir)/libfaad/libfaad.la \
       $(top_builddir)/common/mp4ff/libmp4ff.la

libmp4_la_LDFLAGS = -module -avoid-version $(local_LDFLAGS) -lpthread

libmp4_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c
