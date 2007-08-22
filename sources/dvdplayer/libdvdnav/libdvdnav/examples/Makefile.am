include $(top_srcdir)/misc/Makefile.common

AM_CPPFLAGS = -DDVDNAV_COMPILE -I$(top_srcdir)/src -I$(top_srcdir)/src/dvdread

DVDNAV_LIB = $(top_builddir)/src/libdvdnav.la

noinst_PROGRAMS = menus

menus_SOURCES = menus.c
menus_LDFLAGS = -L$(top_srcdir)/src/
menus_LDADD   = $(DVDNAV_LIB)

$(DVDNAV_LIB):
	@cd $(top_builddir)/src/ && $(MAKE) libdvdnav.la
