
noinst_PROGRAMS = csstest #dvd_region

csstest_SOURCES = csstest.c
csstest_LDADD = $(top_builddir)/src/libdvdcss.la $(bsdi_libadd)
csstest_CFLAGS = -I$(top_srcdir)/src

#dvd_region_SOURCES = dvd_region.c
#dvd_region_LDADD = $(top_builddir)/src/libdvdcss.la
#dvd_region_CFLAGS = -I$(top_srcdir)/src

if SYS_BSDI_LIBDVD
bsdi_libadd = -ldvd
endif

