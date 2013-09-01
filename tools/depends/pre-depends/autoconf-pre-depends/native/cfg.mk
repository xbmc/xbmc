# Customize maint.mk for Autoconf.            -*- Makefile -*-
# Copyright (C) 2003, 2004, 2006, 2008, 2009, 2010 Free Software
# Foundation, Inc.

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

# This file is '-include'd into GNUmakefile.

# Build with our own versions of these tools, when possible.
export PATH = $(shell echo "`pwd`/tests:$$PATH")

# Remove the autoreconf-provided INSTALL, so that we regenerate it.
_autoreconf = autoreconf -i -v && rm -f INSTALL

# Used in maint.mk's web-manual rule
manual_title = Creating Automatic Configuration Scripts

# The local directory containing the checked-out copy of gnulib used in this
# release (override the default).
gnulib_dir = '$(abs_srcdir)'/../gnulib

# The bootstrap tools (override the default).
bootstrap-tools = automake

# Set preferred lists for announcements.

announcement_Cc_ = $(PACKAGE_BUGREPORT), autotools-announce@gnu.org
announcement_mail-alpha = autoconf@gnu.org
announcement_mail-beta = autoconf@gnu.org
announcement_mail-stable = info-gnu@gnu.org, autoconf@gnu.org
announcement_mail_headers_ =						\
To: $(announcement_mail-$(RELEASE_TYPE))				\
CC: $(announcement_Cc_)							\
Mail-Followup-To: autoconf@gnu.org

# Update files from gnulib.
.PHONY: fetch gnulib-update autom4te-update
fetch: gnulib-update autom4te-update

gnulib-update:
	cp $(gnulib_dir)/build-aux/announce-gen $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/config.guess $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/config.sub $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/elisp-comp $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/gendocs.sh $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/git-version-gen $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/gnupload $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/install-sh $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/mdate-sh $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/missing $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/move-if-change $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/texinfo.tex $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/update-copyright $(srcdir)/build-aux
	cp $(gnulib_dir)/build-aux/vc-list-files $(srcdir)/build-aux
	cp $(gnulib_dir)/doc/fdl.texi $(srcdir)/doc
	cp $(gnulib_dir)/doc/gendocs_template $(srcdir)/doc
	cp $(gnulib_dir)/doc/gnu-oids.texi $(srcdir)/doc
	cp $(gnulib_dir)/doc/make-stds.texi $(srcdir)/doc
	cp $(gnulib_dir)/doc/standards.texi $(srcdir)/doc
	cp $(gnulib_dir)/m4/autobuild.m4 $(srcdir)/m4
	cp $(gnulib_dir)/top/GNUmakefile $(srcdir)

WGET = wget
WGETFLAGS = -C off

## Fetch the latest versions of files we care about.
automake_gitweb = \
  http://git.savannah.gnu.org/gitweb/?p=automake.git;a=blob_plain;hb=HEAD;
autom4te_files = \
  Autom4te/Configure_ac.pm \
  Autom4te/Channels.pm \
  Autom4te/FileUtils.pm \
  Autom4te/Struct.pm \
  Autom4te/XFile.pm

move_if_change = '$(abs_srcdir)'/build-aux/move-if-change

autom4te-update:
	rm -fr Fetchdir > /dev/null 2>&1
	mkdir -p Fetchdir/Autom4te
	for file in $(autom4te_files); do \
	  infile=`echo $$file | sed 's/Autom4te/Automake/g'`; \
	  $(WGET) $(WGET_FLAGS) \
	    "$(automake_gitweb)f=lib/$$infile" \
	    -O "Fetchdir/$$file" || exit; \
	done
	perl -pi -e 's/Automake::/Autom4te::/g' Fetchdir/Autom4te/*.pm
	for file in $(autom4te_files); do \
	  $(move_if_change) Fetchdir/$$file $(srcdir)/lib/$$file || exit; \
	done
	rm -fr Fetchdir > /dev/null 2>&1

# Tests not to run.
local-checks-to-skip ?= \
  changelog-check sc_unmarked_diagnostics

# Always use longhand copyrights.
update-copyright-env = \
  UPDATE_COPYRIGHT_USE_INTERVALS=0 \
  UPDATE_COPYRIGHT_MAX_LINE_LENGTH=72

# Prevent incorrect NEWS edits.
old_NEWS_hash = 2ddcbbdee88e191370a07c8d73d8680c
