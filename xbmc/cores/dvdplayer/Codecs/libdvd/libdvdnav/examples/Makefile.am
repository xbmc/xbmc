include $(top_srcdir)/misc/Makefile.common

AM_CPPFLAGS = -DDVDNAV_COMPILE $(DVDREAD_CFLAGS) -I$(top_srcdir)/src

DVDNAV_LIB = $(top_builddir)/src/libdvdnav.la

noinst_PROGRAMS = menus

menus_SOURCES = menus.c
menus_LDADD   = $(DVDNAV_LIB) $(DVDREAD_LIBS)

$(DVDNAV_LIB):
	@cd $(top_builddir)/src/ && $(MAKE) libdvdnav.la
