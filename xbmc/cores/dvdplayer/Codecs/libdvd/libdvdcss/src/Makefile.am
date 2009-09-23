SUBDIRS = dvdcss

lib_LTLIBRARIES = libdvdcss.la

libdvdcss_la_SOURCES = \
	libdvdcss.c libdvdcss.h \
	device.c device.h \
	css.c css.h csstables.h \
	ioctl.c ioctl.h \
	error.c \
	common.h \
	$(bsdi_sources)

EXTRA_libdvdcss_la_SOURCES = \
	$(bsdi_extras)

libdvdcss_la_LIBADD = $(bsdi_libadd)
libdvdcss_la_LDFLAGS = -version-info 3:0:1 $(libtool_flags) @DVDCSS_LDFLAGS@

bsdi_extras = bsdi_ioctl.c bsdi_dvd.h

if SYS_BSDI
bsdi_sources = $(bsdi_extras)
endif

if SYS_BSDI_LIBDVD
bsdi_libadd = -ldvd
endif

if SYS_MSVC
else
libtool_flags = -no-undefined
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdvdcss.pc

