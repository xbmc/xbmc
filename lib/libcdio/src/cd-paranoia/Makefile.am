#   $Id: Makefile.am,v 1.13 2005/01/29 08:03:23 rocky Exp $
#
#    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
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
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
###############################################################################

if HAVE_GETOPT_LONG
else
getopt_sources = getopt.c getopt1.c
endif

EXTRA_DIST = getopt.c getopt1.c \
	     usage.txt.in pod2c.pl \
             doc/FAQ.txt doc/overlapdef.txt


noinst_HEADERS = getopt.h header.h report.h 

transform = s,cd-paranoia,@CDPARANOIA_NAME@,

if BUILD_CD_PARANOIA

SUBDIRS = doc

cd_paranoia_SOURCES = cd-paranoia.c \
	buffering_write.c buffering_write.h \
	$(getopt_sources) header.c report.c usage.h utils.h version.h

cd_paranoia_LDADD =  $(LIBCDIO_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_PARANOIA_LIBS)

bin_PROGRAMS = cd-paranoia

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

# These are the implict rules we've got here.
SUFFIXES = .txt .h

# The dependency on usage.h is by cd-paranoia.o or cd-paranoia.lo or
# is it cd-paranoia.obj? Since we have all of that variation and no
# variation with cd-paranoia.c we'll use that, even  though it is a not
# really correct.
cd-paranoia.c: usage.h

.txt.h:  $(srcdir)/pod2c.pl
	$(PERL) $(srcdir)/pod2c.pl $< >$@

endif

MOSTLYCLEANFILES = usage.h usage.txt

