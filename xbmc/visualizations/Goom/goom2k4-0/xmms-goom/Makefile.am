# the xmms plugin

if HAVE_XMMS
xmms_lib_LTLIBRARIES = libxmmsgoom2.la
xmms_libdir = $(XMMS_VISUALIZATION_PLUGIN_DIR)
libxmmsgoom2_la_LIBADD = $(top_builddir)/src/libgoom2.la $(XMMS_LIBS)
libxmmsgoom2_la_SOURCES = xmms_goom.c
INCLUDES=-DDATADIR=\"@XMMS_DATA_DIR@\" @XMMS_CFLAGS@ -Wall -I../src/
endif

libxmmsgoom2_la_LDFLAGS = -module -avoid-version
