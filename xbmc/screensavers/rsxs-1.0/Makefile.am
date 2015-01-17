## Process this file with automake to produce Makefile.in

SUBDIRS = lib libltdl scripts src config
EXTRA_DIST = THANKS m4/gnulib-cache.m4
DISTCHECK_CONFIGURE_FLAGS = --without-xscreensaver
ACLOCAL_AMFLAGS = -I m4

AM_ETAGSFLAGS = --extra=fq

dist_noinst_DATA = rsxs.spec
rsxs.spec: ChangeLog

srpm: dist
	rpmbuild -ts $(PACKAGE)-$(VERSION).tar.gz
