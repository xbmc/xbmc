## Process this file with automake to produce Makefile.in

SUBDIRS = lib libltdl src config
EXTRA_DIST = scripts THANKS
DISTCHECK_CONFIGURE_FLAGS = --without-xscreensaver
ACLOCAL_AMFLAGS = -I m4

AM_ETAGSFLAGS = --extra=fq

dist_noinst_DATA = rsxs.spec
rsxs.spec: ChangeLog scripts/install.awk scripts/install.awk scripts/verify.awk

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz

mock: dist
	TOPDIR="`rpm -E %_topdir`" || exit 1 && \
	rpmbuild --define='fedora 4' --define='dist .fc4' --nodeps -ts $(PACKAGE)-$(VERSION).tar.gz && \
	mockbuild -r fedora-4-i386-core.cfg --debug "$$TOPDIR/SRPMS/$(PACKAGE)-$(VERSION)-$(RELEASE).fc4.src.rpm" && \
	rpmbuild --define='fedora 5' --define='dist .fc5' --nodeps -ts $(PACKAGE)-$(VERSION).tar.gz && \
	mockbuild -r fedora-5-i386-core.cfg --debug "$$TOPDIR/SRPMS/$(PACKAGE)-$(VERSION)-$(RELEASE).fc5.src.rpm"

if HACKMODE
install-data-hook:
	@$(NORMAL_INSTALL)
	if test -f $(defaultdir)/XScreenSaver; then \
		TMPFILE=`mktemp /tmp/$$$$.XXXXXX` || exit 1 && \
		trap "rm -f $$TMPFILE" 0 && \
		$(AWK) -v 'skip=@DISABLED_HACKS@' -f $(srcdir)/scripts/install.awk < $(defaultdir)/XScreenSaver > $$TMPFILE && \
		$(mkinstalldirs) $(DESTDIR)$(defaultdir) && \
		$(SHELL) $(install_sh_DATA) $$TMPFILE $(DESTDIR)$(defaultdir)/XScreenSaver; \
	fi

uninstall-hook:
	@$(NORMAL_UNINSTALL)
	if test -f $(defaultdir)/XScreenSaver; then \
		TMPFILE=`mktemp /tmp/$$$$.XXXXXX` || exit 1 && \
		trap "rm -f $$TMPFILE" 0 && \
		$(AWK) -v 'skip=@DISABLED_HACKS@' -f $(srcdir)/scripts/uninstall.awk < $(defaultdir)/XScreenSaver > $$TMPFILE && \
		$(mkinstalldirs) $(DESTDIR)$(defaultdir) && \
		$(SHELL) $(install_sh_DATA) $$TMPFILE $(defaultdir)/XScreenSaver; \
	fi

else !HACKMODE
install-data-hook:

uninstall-hook:

endif !HACKMODE
