#   $Id: Makefile.am,v 1.33 2005/01/27 03:26:37 rocky Exp $
#
#    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
## Process this file with automake to produce Makefile.in
##   which configure then turns into a Makefile  ...
##     which make can then use to produce stuff. Isn't configuration simple? 

EXTRA_DIST = libpopt.m4 THANKS README.libcdio \
	     package/libcdio-suse.spec.in package/libcdio-suse.spec \
	     package/libcdio.spec.in package/libcdio.spec \
	     libcdio.pc.in libcdio.pc \
	     libcdio_cdda.pc.in libcdio_cdda.pc \
	     libiso9660.pc.in libiso9660.pc \
	     example/README \
	     MSVC/config.h    MSVC/README MSVC/cd-info.vcproj \
             MSVC/libcdio.sln MSVC/libcdio.vcproj 

SUBDIRS =  doc include lib src test example

# pkg-config(1) related rules
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcdio.pc libcdio_paranoia.pc \
	         libcdio_cdda.pc libiso9660.pc 

$(pkgconfig_DATA): config.status

# List of additional files for expanded regression tests
DISTFILES_REGRESSION = tests/monvoisin.nrg  tests/monvoisin.right \
		       tests/svcdgs.nrg  tests/svcdgs.nrg \
	               tests/svcd_ogt_test_ntsc.bin \
	               tests/svcd_ogt_test_ntsc.cue \
	               tests/svcd_ogt_test_ntsc.right \
		       tests/vcd_demo.bin tests/vcd_demo.cue \
		       tests/vcd_demo.right

REGRESSION_VERSION = 1.1

distdir_regression = ../$(PACKAGE)-$(REGRESSION_VERSION)-tests

test: check

doxygen:
	-( cd ${top_srcdir}/doc/doxygen && /bin/sh ${srcdir}/run_doxygen )

dist-regression: distdir-regression
	cd $(distdir) && $(AMTAR) chof - tests | GZIP=$(GZIP_ENV) gzip -c >$(distdir_regression).tar.gz
	$(am__remove_distdir)

distdir-regression: $(DISTFILES_REGRESSION)
	$(am__remove_distdir)
	mkdir $(distdir)
	@list='$(DISTFILES_REGRESSION)'; for file in $$list; do \
	  if test -f $$file || test -d $$file; then d=.; else d=$(srcdir); fi; \
	  dir=`echo "$$file" | sed -e 's,/[^/]*$$,,'`; \
	  if test "$$dir" != "$$file" && test "$$dir" != "."; then \
	    dir="/$$dir"; \
	    $(mkinstalldirs) "$(distdir)$$dir"; \
	  else \
	    dir=''; \
	  fi; \
	  if test -d $$d/$$file; then \
	    if test -d $(srcdir)/$$file && test $$d != $(srcdir); then \
	      cp -pR $(srcdir)/$$file $(distdir)$$dir || exit 1; \
	    fi; \
	    cp -pR $$d/$$file $(distdir)$$dir || exit 1; \
	  else \
	    test -f $(distdir_regressoin)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
	-find $(distdir) -type d ! -perm -777 -exec chmod a+rwx {} \; -o \
	  ! -type d ! -perm -444 -links 1 -exec chmod a+r {} \; -o \
	  ! -type d ! -perm -400 -exec chmod a+r {} \; -o \
	  ! -type d ! -perm -444 -exec $(SHELL) $(install_sh) -c -m a+r {} {} \; \
	|| chmod -R a+r $(distdir)

check_nrg.sh: $(top_builddir)/config.status check_nrg.sh.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@
	chmod +x config_nrg.sh
check_cue.sh: $(top_builddir)/config.status check_cue.sh.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@
	chmod +x config_cue.sh

check_iso.sh: $(top_builddir)/config.status check_iso.sh.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@
	chmod +x config_iso.sh

make-executable:

# cvs2cl
MAINTAINERCLEANFILES = ChangeLog

if MAINTAINER_MODE

.PHONY: ChangeLog
ChangeLog:
	$(CVS2CL) -W 450 --header $(srcdir)/cvs2cl_header --utc -w -I ChangeLog --usermap $(srcdir)/cvs2cl_usermap -P 

ACLOCAL_AMFLAGS=-I .

endif
