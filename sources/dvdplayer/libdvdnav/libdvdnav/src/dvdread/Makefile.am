include $(top_srcdir)/misc/Makefile.common

includedir = ${prefix}/include/dvdnav

AM_CPPFLAGS = -DDVDNAV_COMPILE -I$(top_srcdir)/src/vm

noinst_LTLIBRARIES = libdvdread.la

libdvdread_la_SOURCES = dvd_reader.c nav_read.c ifo_read.c \
	dvd_input.c dvd_udf.c md5.c nav_print.c ifo_print.c \
	bswap.h dvd_input.h dvdread_internal.h dvd_udf.h md5.h

libdvdread_la_LIBADD = $(DYNAMIC_LD_LIBS)

include_HEADERS = dvd_reader.h nav_read.h ifo_read.h \
	nav_print.h ifo_print.h ifo_types.h nav_types.h
