include $(top_srcdir)/misc/Makefile.common

includedir = ${prefix}/include/dvdnav

AM_CPPFLAGS = -DDVDNAV_COMPILE $(THREAD_CFLAGS) \
	-I$(top_srcdir)/src -I$(top_srcdir)/src/dvdread

noinst_LTLIBRARIES = libdvdvm.la

libdvdvm_la_SOURCES = decoder.c vm.c vmcmd.c

libdvdvm_la_LDFLAGS = $(THREAD_LIBS)

include_HEADERS = 

noinst_HEADERS = decoder.h vm.h vmcmd.h
