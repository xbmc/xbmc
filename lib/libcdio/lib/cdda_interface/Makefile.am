#   $Id: Makefile.am,v 1.3 2005/01/15 16:05:44 rocky Exp $
#
#    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
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
# Things to make the cdda_interface library
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

libcdio_cdda_la_CURRENT := 0
libcdio_cdda_la_REVISION := 0
libcdio_cdda_la_AGE := 0

noinst_HEADERS  = common_interface.h drive_exceptions.h low_interface.h \
		  smallft.h utils.h

libcdio_cdda_sources =  common_interface.c cooked_interface.c interface.c \
	scan_devices.c	smallft.c toc.c utils.c

lib_LTLIBRARIES = libcdio_cdda.la

libcdio_cdda_la_SOURCES = $(libcdio_cdda_sources)
libcdio_cdda_la_ldflags = -version-info $(libcdio_cdda_la_CURRENT):$(libcdio_cdda_la_REVISION):$(libcdio_cdda_la_AGE)

libcdio_cdda_la_LDFLAGS = $(libcdio_cdda_la_ldflags)

INCLUDES = $(LIBCDIO_CFLAGS)

FLAGS=@LIBCDIO_CFLAGS@ @UCDROM_H@ @TYPESIZES@ @CFLAGS@

OPT=$(FLAGS)
DEBUG=$(FLAGS) -DCDDA_TEST

## test:	
##	$(MAKE) libcdio_cdda.a CFLAGS="$(DEBUG)"
##	$(CC) $(DEBUG) -c test_interface.c
##	$(LD) $(DEBUG) test_interface.o $(LDFLAGS) -o cdda_test $(LIBS) libcdio_cdda.a

LIBS = $(LIBCDIO_LIBS)
