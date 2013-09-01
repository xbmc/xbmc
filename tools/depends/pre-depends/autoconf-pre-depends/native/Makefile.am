# Make Autoconf.

# Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007, 2008,
# 2009, 2010 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# bin/ must be run first, as it builds executables needed for tests.
# autom4te uses autotest.m4f to generate `testsuite', so build tests last.
# Rules in man/ use scripts from both bin/ and tests/, so *it* goes last.
SUBDIRS = bin . lib doc tests man

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = ChangeLog.0 ChangeLog.1 ChangeLog.2 \
	     BUGS COPYINGv3 COPYING.EXCEPTION \
	     GNUmakefile maint.mk cfg.mk \
	     build-aux/gendocs.sh \
	     build-aux/git-version-gen \
	     build-aux/announce-gen build-aux/gnupload \
	     .prev-version .version

## --------- ##
## INSTALL.  ##
## --------- ##

AM_MAKEINFOFLAGS = --no-headers --no-validate --no-split

if MAKE_CASE_SENSITIVE
pkgdata_DATA = $(srcdir)/INSTALL

MAINTAINERCLEANFILES = $(srcdir)/INSTALL
$(srcdir)/INSTALL: $(top_srcdir)/doc/install.texi
	$(MAKEINFO) $(AM_MAKEINFOFLAGS) $(MAKEINFOFLAGS) --plaintext -o $@ \
	  $(top_srcdir)/doc/install.texi
endif


# Version string management.  There are two files to be aware of:
# .tarball-version - present only in a distribution tarball, and not in
#   a checked-out repository.  Created with contents that were learned at
#   the last time autoconf was run, and used by git-version-gen.  Must not
#   be present in either $(srcdir) or $(builddir) for git-version-gen to
#   give accurate answers during normal development with a checked out tree,
#   but must be present in a tarball when there is no version control system.
#   Therefore, it cannot be used in any dependencies.  GNUmakefile has
#   hooks to force a reconfigure at distribution time to get the value
#   correct, without penalizing normal development with extra reconfigures.
# .version - present in a checked-out repository and in a distribution
#   tarball.  At least as current as the most recent .tarball-version
#   creation.  Usable in dependencies, particularly for files that don't
#   want to depend on config.h but do want to track version changes.
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

# Arrange so that .tarball-version appears only in distribution tarballs,
# never in a checked-out repository.
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

# Arrange to remove the symlink to GNUmakefile in VPATH builds.
# TODO remove this once automake vs. AC_CONFIG_LINKS issue is fixed.
distclean-local:
	if test x"$(VPATH)" != x ; then rm -f GNUmakefile ; fi

# Perl coverage statistics.
PERL_COVERAGE_DB = `pwd`/cover_db
PERL_COVERAGE_FLAGS = -MDevel::Cover=-db,$(PERL_COVERAGE_DB),-silent,on,-summary,off
PERL_COVER = cover

check-coverage-run: all
	$(mkinstalldirs) $(PERL_COVERAGE_DB)
	PERL5OPT="$(PERL_COVERAGE_FLAGS)"; export PERL5OPT; \
	$(MAKE) $(AM_MAKEFLAGS) check

check-coverage-report:
	@if test ! -d $(PERL_COVERAGE_DB); then \
	  echo "No coverage database found in \`$(PERL_COVERAGE_DB)'." >&2; \
	  echo "Please run \`make check-coverage' first" >&2; \
	  exit 1; \
	fi
	$(PERL_COVER) $(PERL_COVERAGE_DB) $(PERL_COVER_FLAGS)

# We don't use direct dependencies here because we'd like to be able
# to invoke the report even after interrupted check-coverage.
check-coverage: check-coverage-run
	$(MAKE) $(AM_MAKEFLAGS) check-coverage-report

clean-local: clean-coverage
clean-coverage:
	rm -rf $(PERL_COVERAGE_DB)

.PHONY: check-coverage check-coverage-run check-coverage-report clean-coverage
