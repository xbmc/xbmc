ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = msvc libdvdcss.spec doc/header.html doc/footer.html INSTALL

dist_doc_DATA = AUTHORS COPYING NEWS README ChangeLog

DISTCLEANFILES = 

lib_LTLIBRARIES = libdvdcss.la
EXTRA_PROGRAMS = csstest dvd_region

libdvdcss_la_SOURCES = \
	src/libdvdcss.c \
	src/libdvdcss.h \
	src/device.c \
	src/device.h \
	src/css.c \
	src/css.h \
	src/csstables.h \
	src/ioctl.c \
	src/ioctl.h \
	src/error.c \
	src/common.h

libdvdcss_la_LDFLAGS = -version-info $(DVDCSS_LTVERSION) $(DVDCSS_LDFLAGS)
libdvdcss_la_LIBADD =

if !SYS_MSVC
libdvdcss_la_LDFLAGS += = -no-undefined
endif

csstest_SOURCES = test/csstest.c
csstest_LDADD = libdvdcss.la
csstest_CFLAGS = -I $(top_srcdir)/src

dvd_region_SOURCES = test/dvd_region.c src/ioctl.c
dvd_region_LDADD = libdvdcss.la
dvd_region_CFLAGS = -I $(top_srcdir)/src

pkgincludedir = $(includedir)/dvdcss
pkginclude_HEADERS = src/dvdcss/dvdcss.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libdvdcss.pc

if APIDOC
all-local: apidoc
install-data-local: install-apidoc
uninstall-hook: uninstall-apidoc
endif

apidoc: stamp-doxygen

stamp-doxygen: doc/doxygen.cfg
	cd doc && doxygen doxygen.cfg
	touch $@

clean-local:
	-rm -Rf stamp-doxygen doc/html

install-apidoc:
	$(MKDIR_P) $(DESTDIR)$(htmldir)
	for file in doc/html/*; do \
		$(INSTALL_DATA) $$file "$(DESTDIR)$(htmldir)" || exit 1; \
	done;

uninstall-apidoc:
	-rm -Rf $(DESTDIR)$(htmldir)/*.png $(DESTDIR)$(htmldir)/*.html $(DESTDIR)$(htmldir)/*.css $(DESTDIR)$(htmldir)/*.js
