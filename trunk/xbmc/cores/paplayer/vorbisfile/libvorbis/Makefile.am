## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6 foreign dist-zip dist-bzip2

SUBDIRS = lib include doc examples vq

m4datadir = $(datadir)/aclocal
m4data_DATA = vorbis.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vorbis.pc vorbisenc.pc vorbisfile.pc

EXTRA_DIST = \
	CHANGES COPYING \
	todo.txt autogen.sh \
	libvorbis.spec libvorbis.spec.in \
	vorbis.m4 \
	vorbis.pc.in vorbisenc.pc.in vorbisfile.pc.in \
	vorbis-uninstalled.pc.in \
	vorbisenc-uninstalled.pc.in \
	vorbisfile-uninstalled.pc.in \
	doc examples symbian vq \
	macos macosx win32


DISTCHECK_CONFIGURE_FLAGS = --enable-docs

dist-hook:
	for item in $(EXTRA_DIST); do \
	  if test -d $$item; then \
	    echo -n "cleaning $$item dir for distribution..."; \
	    rm -rf `find $(distdir)/$$item -name .svn`; \
	    echo "OK"; \
	  fi; \
	done

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
