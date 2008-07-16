## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign 1.6 dist-zip

SUBDIRS = src include doc win32

m4datadir = $(datadir)/aclocal
m4data_DATA = ogg.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ogg.pc

# we include the whole debian/ dir in EXTRA_DIST because there's a problem
# with autotools and HFS+ MacOSX file systems that caused debian/Makefile.am
# to pick up on the lowercase changelog file and add ChangeLog to DIST_COMMON
# because of it, breaking make dist.  This works just as well.
EXTRA_DIST = README AUTHORS CHANGES COPYING \
	libogg.spec libogg.spec.in \
	ogg.m4 ogg.pc.in ogg-uninstalled.pc.in \
	macos macosx debian

dist-hook:
	rm -rf `find $(distdir)/debian -name .svn`
	rm -rf `find $(distdir)/debian -name "Makefile*"`
	rm -rf `find $(distdir)/macos -name .svn`
	rm -rf `find $(distdir)/macosx -name .svn`

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
