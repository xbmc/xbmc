# Make Autoconf documentation.

# Copyright (C) 2000, 2001, 2002, 2003, 2007, 2008, 2009, 2010 Free
# Software Foundation, Inc.

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

AM_MAKEINFOFLAGS = --no-split
TEXI2HTML_FLAGS = -split_chapter
TEXINFO_TEX = ../build-aux/texinfo.tex

info_TEXINFOS = autoconf.texi standards.texi
autoconf_TEXINFOS = fdl.texi install.texi
standards_TEXINFOS = fdl.texi gnu-oids.texi make-stds.texi

EXTRA_DIST = gendocs_template

# Files from texi2dvi that should be removed, but which Automake does
# not know.
CLEANFILES = autoconf.ACs  autoconf.cvs  autoconf.MSs  autoconf.prs \
	     autoconf.ATs  autoconf.evs  autoconf.fns  autoconf.ovs \
	     autoconf.ca   autoconf.CA   autoconf.cas  autoconf.CAs \
	     autoconf.tmp
