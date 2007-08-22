libdir = `$(XMMS_CONFIG) --input-plugin-dir`
lib_LTLIBRARIES = libmp4.la

libmp4_la_CFLAGS = `$(XMMS_CONFIG) --cflags` -Wall \
       `$(GTK_CONFIG) --cflags` -DHAVE_GLIB_H=1 \
       -I$(top_srcdir)/include -I$(top_srcdir)/common/mp4v2

libmp4_la_LIBADD = $(top_builddir)/libfaad/libfaad.la \
       $(top_builddir)/common/mp4v2/libmp4v2.la

libmp4_la_LDFLAGS = -module -avoid-version `$(XMMS_CONFIG) --libs` \
       `$(GTK_CONFIG) --libs` -lpthread -lstdc++
libmp4_la_SOURCES = libmp4.c mp4_utils.c aac_utils.c
