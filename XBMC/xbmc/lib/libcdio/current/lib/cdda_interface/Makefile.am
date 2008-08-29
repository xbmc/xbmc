#   $Id: Makefile.am,v 1.12 2007/10/15 04:53:59 rocky Exp $
#
#    Copyright (C) 2004, 2005, 2006, 2007 Rocky Bernstein <rocky@gnu.org>
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
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#    02110-1301 USA.
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

EXTRA_DIST = libcdio_cdda.sym

libcdio_cdda_la_CURRENT = 0
libcdio_cdda_la_REVISION = 3
libcdio_cdda_la_AGE = 0

noinst_HEADERS  = common_interface.h drive_exceptions.h low_interface.h \
		  smallft.h utils.h

libcdio_cdda_sources =  common_interface.c cddap_interface.c interface.c \
	scan_devices.c	smallft.c toc.c utils.c

lib_LTLIBRARIES = libcdio_cdda.la

libcdio_cdda_la_SOURCES = $(libcdio_cdda_sources)
libcdio_cdda_la_ldflags = -version-info $(libcdio_cdda_la_CURRENT):$(libcdio_cdda_la_REVISION):$(libcdio_cdda_la_AGE)

INCLUDES = $(LIBCDIO_CFLAGS)

FLAGS=@LIBCDIO_CFLAGS@ @UCDROM_H@ @TYPESIZES@ @CFLAGS@

OPT=$(FLAGS)
DEBUG=$(FLAGS) -DCDDA_TEST

## test:	
##	$(MAKE) libcdio_cdda.a CFLAGS="$(DEBUG)"
##	$(CC) $(DEBUG) -c test_interface.c
##	$(LD) $(DEBUG) test_interface.o $(LDFLAGS) -o cdda_test $(LIBS) libcdio_cdda.a

LIBS = $(LIBCDIO_LIBS) @COS_LIB@


########################################################
# Things to version the symbols in the libraries
########################################################

# An explanation of the versioning problem from Nicolas Boullis and
# the versioned symbol solution he uses below...
# 
# Currently, libvcdinfo uses the cdio_open function from libcdio.
# Let's imagine a program foobar that uses both the vcdinfo_open
# function from libvcdinfo and the cdio_open function from libcdio.

# Currently, libcdio has SONAME libcdio.so.0, libvcdinfo has SONAME
# libvcdinfo.so.0 and requires libcdio.so.0, and foobar requires both
# libvcdinfo.so.0 and libcdio.so.0. Everything looks fine.
#
# Now, for some reason, you decide to change the cdio_open function.
# That's your right, but you have to bump the CURRENT version and (if I
# understand it correctly, athough this is not that clear in libtool's
# documentation) set the AGE to 0. Anyway, this bumps the SONAME, which is
# sane since the interface changes incompatibly.

# Now, you have a new libcdio with SONAME libcdio.so.1. But libvcdinfo and
# foobar still require libcdio.so.0. Everything is still fine.

# Now, after some minor changes, the author of foobar recompiles foobar.
# Then, foobar now requires libvcdinfo.so.0 and libcdio.so.1. And foobar
# now segfaults...

# What is happening? When you run foobar, if brings both libvcdinfo.so.0
# and libcdio.so.1, but libvcdinfo.so.0 also brings libcdio.so.0. So you
# have both libcdio.so.0 and libcdio.so.1 that bring their symbols to the
# global namespace. Hence, you have to incompatible versions of the
# cdio_open function in the name space. When foobar calls cdio_open, it
# may choose the wrong function, and segfaults...

# With versioned symbols, the cdio_open function from libcdio.so.0 may be
# known as (something that looks like) cdio_open@@CDIO_0. An the cdio_open
# function from libcdio.so.1 as cdio_open@@CDIO_1. Both versions of
# libcdio would still be brought in by the most recent foobar, but foobar
# (and libvcdinfo) know which versioned function to use and then use the
# good one.


# This is some simple versioning where every symbol is versioned with
# something that looks like the SONAME of the library. More complex (and
# better) versioning is possible; it is for example what is used by glibc.
# But good complex versioning is something that requires much more
# work...


# The below is a impliments symbol versioning.  First of all, I
# compute MAJOR as CURENT - AGE; that is what is used within libtool
# (at least on GNU/Linux systems) for the number in the SONAME.  The
# nm command gives the list of symbols known in each of the object
# files that will be part of the shared library. And the sed command
# extracts from this list those symbols that will be shared. (This sed
# command comes from libtool.)

libcdio_cdda_la_MAJOR = $(shell expr $(libcdio_cdda_la_CURRENT) - $(libcdio_cdda_la_AGE))
if BUILD_VERSIONED_LIBS
libcdio_cdda_la_LDFLAGS = $(libcdio_cdda_la_ldflags) -Wl,--version-script=libcdio_cdda.la.ver
libcdio_cdda_la_DEPENDENCIES = libcdio_cdda.la.ver

libcdio_cdda.la.ver: $(libcdio_cdda_la_OBJECTS) $(srcdir)/libcdio_cdda.sym
	echo 'CDIO_CDDA_$(libcdio_cdda_la_MAJOR) { ' > $@
	objs=`for obj in $(libcdio_cdda_la_OBJECTS); do sed -ne "s/^pic_object='\(.*\)'$$/\1/p" $$obj; done`; \
	if test -n "$$objs" ; then \
	  nm $${objs} | sed -n -e 's/^.*[ 	][ABCDGIRSTW][ABCDGIRSTW]*[ 	][ 	]*\([_A-Za-z][_A-Za-z0-9]*\)$$/\1/p' | sort -u | { first=true; while read symbol; do if grep -q "^$${symbol}\$$" $(srcdir)/libcdio_cdda.sym; then if test $$first = true; then echo "  global:"; first=false; fi; echo "    $${symbol};"; fi; done; } >> $@; \
	  nm $${objs} | sed -n -e 's/^.*[ 	][ABCDGIRSTW][ABCDGIRSTW]*[ 	][ 	]*\([_A-Za-z][_A-Za-z0-9]*\)$$/\1/p' | sort -u | { first=true; while read symbol; do if grep -q "^$${symbol}\$$" $(srcdir)/libcdio_cdda.sym; then :; else if test $$first = true; then echo "  local:"; first=false; fi; echo "    $${symbol};"; fi; done; } >> $@; \
	fi
	echo '};' >> $@
else 
libcdio_cdda_la_LDFLAGS = $(libcdio_cdda_la_ldflags)
endif 

MOSTLYCLEANFILES = libcdio_cdda.la.ver
