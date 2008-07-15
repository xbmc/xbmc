## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include doc win32 debian

m4datadir = $(datadir)/aclocal
m4data_DATA = ogg.m4

EXTRA_DIST = README AUTHORS CHANGES COPYING libogg.spec ogg.m4 macos

dist-hook:
	rm -rf `find $(distdir)/macos -name CVS`

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
