#   $Id: Makefile.am,v 1.25 2008/03/15 17:26:13 rocky Exp $
#
#    Copyright (C) 2004, 2005, 2006, 2007, 2008 Rocky Bernstein <rocky@gnu.org>
#    Copyright (C) 1998 Monty xiphmont@mit.edu
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
###############################################################################

transform = s,cd-paranoia,@CDPARANOIA_NAME@,

if BUILD_CD_PARANOIA

SUBDIRS = doc

GETOPT_C = getopt1.c getopt.c

EXTRA_DIST = usage.txt.in usage-copy.h pod2c.pl \
             doc/FAQ.txt doc/overlapdef.txt $(GETOPT_C) getopt.h

noinst_HEADERS = header.h report.h $(GETOPT_H)


cd_paranoia_SOURCES = cd-paranoia.c \
	buffering_write.c buffering_write.h \
	header.c report.c usage.h utils.h version.h $(GETOPT_C)

cd_paranoia_LDADD =  $(LIBCDIO_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_PARANOIA_LIBS) $(LIBICONV)

cd_paranoia_DEPENDENCIES =  $(LIBCDIO_DEPS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_PARANOIA_LIBS)


bin_PROGRAMS = cd-paranoia

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

cd-paranoia.$(OBJEXT): usage.h

if HAVE_PERL
usage.h: usage.txt $(srcdir)/pod2c.pl
	$(PERL) $(srcdir)/pod2c.pl $< >$@
else
usage.h: usage-copy.h
	cp $< $@
endif

endif

MOSTLYCLEANFILES = usage.h usage.txt

