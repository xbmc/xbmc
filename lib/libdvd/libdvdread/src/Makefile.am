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

include_HEADERS = dvdread/dvd_reader.h dvdread/nav_read.h dvdread/ifo_read.h \
	dvdread/nav_print.h dvdread/ifo_print.h dvdread/ifo_types.h dvdread/nav_types.h dvdread/dvd_udf.h dvdread/bitreader.h
