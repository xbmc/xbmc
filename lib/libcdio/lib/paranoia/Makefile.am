#   $Id: Makefile.am,v 1.1 2004/12/18 17:29:32 rocky Exp $
#
#    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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
########################################################
# Things to make the libcdio_paranoia library
########################################################

#
# From libtool documentation amended with guidance from N. Boullis: 
#
#  1. Start with version information of `0:0:0' for each libtool library.
#
#  2. It is probably not a good idea to update the version information 
#     several times between public releases, but rather once per public
#     release. (This seems to be more an aesthetic consideration than
#     a hard technical one.)
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:R+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed or changed since the last
#     public release, then set AGE to 0. A changed interface means an
#     incompatibility with previous versions.

libcdio_paranoia_la_CURRENT := 0
libcdio_paranoia_la_REVISION := 0
libcdio_paranoia_la_AGE := 0

noinst_HEADERS  = gap.h isort.h overlap.h p_block.h

libcdio_paranoia_sources = gap.c isort.c overlap.c overlap.h \
	p_block.c paranoia.c 

lib_LTLIBRARIES = libcdio_paranoia.la

libcdio_paranoia_la_SOURCES = $(libcdio_paranoia_sources)
libcdio_paranoia_la_ldflags = -version-info $(libcdio_paranoia_la_CURRENT):$(libcdio_paranoia_la_REVISION):$(libcdio_paranoia_la_AGE)

libcdio_paranoia_la_LDFLAGS = $(libcdio_paranoia_la_ldflags)

INCLUDES = $(LIBCDIO_CFLAGS)

FLAGS=@LIBCDIO_CFLAGS@ @TYPESIZES@ @CFLAGS@ -I.. -I../..
OPT=$(FLAGS)
DEBUG=$(FLAGS)

## SUFFIXES = .t
## TFILES = isort.t gap.t p_block.t paranoia.t
##test:	$(TFILES)
##.c.t:
##	$(CC) -g -DTEST $(DEBUG) -o $@ $< $(LIBS)
##	$@
##debug: 
##	$(MAKE) libcdio_paranoia.a CFLAGS="$(DEBUG)"

LIBS = $(LIBCDIO_LIBS) $(LIBCDIO_CDDA_LIBS)
