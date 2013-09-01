# Make Autoconf-related libraries.

# Copyright (C) 2001, 2002, 2003, 2004, 2005, 2009, 2010 Free Software
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

SUBDIRS = Autom4te m4sugar autoconf autotest autoscan emacs
nodist_pkgdata_DATA = autom4te.cfg
EXTRA_DIST = autom4te.in freeze.mk

edit = sed \
	-e 's|@SHELL[@]|$(SHELL)|g' \
	-e 's|@PERL[@]|$(PERL)|g' \
	-e 's|@bindir[@]|$(bindir)|g' \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	-e 's|@prefix[@]|$(prefix)|g' \
	-e 's|@autoconf-name[@]|'`echo autoconf | sed '$(transform)'`'|g' \
	-e 's|@autoheader-name[@]|'`echo autoheader | sed '$(transform)'`'|g' \
	-e 's|@autom4te-name[@]|'`echo autom4te | sed '$(transform)'`'|g' \
	-e 's|@M4[@]|$(M4)|g' \
	-e 's|@AWK[@]|$(AWK)|g' \
	-e 's|@VERSION[@]|$(VERSION)|g' \
	-e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'

# All the files below depend on Makefile so that they are rebuilt
# when the prefix, etc. changes. Unfortunately, suffix rules
# cannot have additional dependencies, so we have to use explicit rules.
CLEANFILES = autom4te.cfg
autom4te.cfg: $(srcdir)/autom4te.in Makefile
	rm -f autom4te.cfg autom4te.tmp
	$(edit) $(srcdir)/autom4te.in >autom4te.tmp
	chmod a-w autom4te.tmp
	mv autom4te.tmp autom4te.cfg
