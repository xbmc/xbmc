include $(top_srcdir)/misc/Makefile.common

includedir = ${prefix}/include/dvdread

AM_CPPFLAGS = -I$(top_srcdir)/src

lib_LTLIBRARIES = libdvdread.la

libdvdread_la_SOURCES = dvd_reader.c nav_read.c ifo_read.c \
	dvd_input.c dvd_udf.c md5.c nav_print.c ifo_print.c bitreader.c \
	bswap.h dvd_input.h dvdread_internal.h dvd_udf.h md5.h bitreader.h

libdvdread_la_LIBADD = $(DYNAMIC_LD_LIBS)

libdvdread_la_LDFLAGS = -version-info $(DVDREAD_LT_CURRENT):$(DVDREAD_LT_REVISION):$(DVDREAD_LT_AGE) \
	-export-symbols-regex "(^dvd.*|^nav.*|^ifo.*|^DVD.*|^UDF.*)"

include_HEADERS = dvd_reader.h nav_read.h ifo_read.h \
	nav_print.h ifo_print.h ifo_types.h nav_types.h dvd_udf.h bitreader.h
