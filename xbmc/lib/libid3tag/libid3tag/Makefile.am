##
## libid3tag - ID3 tag manipulation library
## Copyright (C) 2000-2004 Underbit Technologies, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
## $Id: Makefile.am,v 1.26 2004/02/17 02:11:28 rob Exp $
##

## Process this file with automake to produce Makefile.in

SUBDIRS =		
DIST_SUBDIRS =		msvc++

lib_LTLIBRARIES =	libid3tag.la
include_HEADERS =	id3tag.h

## From the libtool documentation on library versioning:
##
## CURRENT
##      The most recent interface number that this library implements.
##
## REVISION
##      The implementation number of the CURRENT interface.
##
## AGE
##      The difference between the newest and oldest interfaces that this
##      library implements.  In other words, the library implements all the
##      interface numbers in the range from number `CURRENT - AGE' to
##      `CURRENT'.
##
##    If two libraries have identical CURRENT and AGE numbers, then the
## dynamic linker chooses the library with the greater REVISION number.
##
##   1. Start with version information of `0:0:0' for each libtool library.
##
##   2. Update the version information only immediately before a public
##      release of your software.  More frequent updates are unnecessary,
##      and only guarantee that the current interface number gets larger
##      faster.
##
##   3. If the library source code has changed at all since the last
##      update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
##
##   4. If any interfaces have been added, removed, or changed since the
##      last update, increment CURRENT, and set REVISION to 0.
##
##   5. If any interfaces have been added since the last public release,
##      then increment AGE.
##
##   6. If any interfaces have been removed since the last public release,
##      then set AGE to 0.

version_current =	3
version_revision =	0
version_age =		3

version_info =		$(version_current):$(version_revision):$(version_age)

EXTRA_DIST =		genre.dat.sed  \
			CHANGES COPYRIGHT CREDITS README TODO VERSION

if DEBUG
debug = debug.c debug.h
else
debug =
endif

libid3tag_la_SOURCES =	version.c ucs4.c latin1.c utf16.c utf8.c  \
			parse.c render.c field.c frametype.c compat.c  \
			genre.c frame.c crc.c util.c tag.c file.c  \
			version.h ucs4.h latin1.h utf16.h utf8.h  \
			parse.h render.h field.h frametype.h compat.h  \
			genre.h frame.h crc.h util.h tag.h file.h  \
			id3tag.h global.h metadata.h metadata.c \ 
			genre.dat $(debug)

EXTRA_libid3tag_la_SOURCES =  \
			frametype.gperf compat.gperf genre.dat.in  \
			debug.c debug.h

libid3tag_la_LDFLAGS =	-version-info $(version_info)

BUILT_SOURCES =		frametype.c compat.c genre.dat

$(srcdir)/frametype.c: $(srcdir)/frametype.gperf Makefile.am
	cd $(srcdir) &&  \
	gperf -tCcTonD -K id -N id3_frametype_lookup -s -3 -k '*'  \
		frametype.gperf |  \
	sed -e 's/\(struct id3_frametype\);/\1/' |  \
	sed -e '/\$$''Id: /s/\$$//g' >frametype.c

$(srcdir)/compat.c: $(srcdir)/compat.gperf Makefile.am
	cd $(srcdir) &&  \
	gperf -tCcTonD -K id -N id3_compat_lookup -s -3 -k '*'  \
		compat.gperf |  \
	sed -e 's/\(struct id3_compat\);/\1/' |  \
	sed -e '/\$$''Id: /s/\$$//g' >compat.c

$(srcdir)/genre.dat: $(srcdir)/genre.dat.in $(srcdir)/genre.dat.sed Makefile.am
	cd $(srcdir) &&  \
	sed -n -f genre.dat.sed genre.dat.in |  \
	sed -e '/\$$''Id: /s/\$$//g' >genre.dat

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

again:
	$(MAKE) clean
	$(MAKE)

.PHONY: again
