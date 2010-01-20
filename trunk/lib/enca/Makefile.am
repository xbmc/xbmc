# @(#) $Id: Makefile.am,v 1.35 2005/11/24 10:21:32 yeti Exp $
if MAINTAINER_MODE
SUBDIRS = tools data script lib src devel-docs test
else
SUBDIRS = tools script lib src devel-docs test
endif
man_MANS = man/enca.1

DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-mode

ACLOCAL_AMFLAGS = -I m4

M4TESTS = \
  m4/crash-me \
  m4/gtk-doc.m4 \
  m4/libiconv.m4 \
  m4/libm.m4 \
  m4/librecode.m4 \
  m4/localias.m4 \
  m4/long-text.l2 \
  m4/recode-bugs.m4 \
  m4/scanf.m4 \
  m4/tools.m4 \
  m4/typevar.m4

EXTRA_DIST = \
  FAQ \
  README.devel \
  TODO \
  ChangeLog.prelib \
  autogen.sh \
  config.rpath \
  enca.pc.in \
  enca.spec \
  enca.spec.in \
  iconvcap.c \
  topline.sh \
  man/enca.1 \
  $(M4TESTS)

####################### maintainer's targets ######################
# alias test=check
test: check

docs:
	cd $(srcdir)/devel-docs && $(MAKE) docs

.PHONY: test docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = enca.pc

$(pkgconfig_DATA): config.status

install-data-hook:
	cd $(DESTDIR)$(man1dir); \
	inst1=`echo enca | sed '$(transform)'`.1; \
	inst2=`echo enconv | sed '$(transform)'`.1; \
	$(LN_S) $$inst1 $$inst2

uninstall-hook:
	inst=`echo enconv | sed '$(transform)'`.1; \
	rm -f $(DESTDIR)$(man1dir)/$$inst

clean-local:
	rm -f iconvcap iconvcap.o *~ core.*

distclean-local:
	rm -f iconvenc.h

if MAINTAINER_MODE
dist-check-maintainer:
else
dist-check-maintainer:
	@echo "*** maintainer mode must be enabled in order to make dist"
	@false
endif

dist-hook: dist-check-maintainer
	cat $(distdir)/tools/iconvenc.null >$(distdir)/iconvenc.h

if MAINTAINER_MODE
# To be run [by hand] before release (not the same as before making dists).
update:
	./topline.sh

endif
.PHONY: update

