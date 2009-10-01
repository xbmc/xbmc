##
## libmad - MPEG audio decoder library
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
## $Id: Makefile.am,v 1.23 2004/02/17 02:02:03 rob Exp $
##

## Process this file with automake to produce Makefile.in

SUBDIRS =		
DIST_SUBDIRS =		msvc++

lib_LTLIBRARIES =	libmad.la
include_HEADERS =	mad.h

EXTRA_PROGRAMS =	minimad

minimad_SOURCES =	minimad.c
minimad_INCLUDES =	
minimad_LDADD =		libmad.la

EXTRA_DIST =		mad.h.sed  \
			CHANGES COPYRIGHT CREDITS README TODO VERSION

exported_headers =	version.h fixed.h bit.h timer.h stream.h frame.h  \
			synth.h decoder.h

headers =		$(exported_headers)  \
			global.h layer12.h layer3.h huffman.h

data_includes =		D.dat imdct_s.dat qc_table.dat rq_table.dat  \
			sf_table.dat

libmad_la_SOURCES =	version.c fixed.c bit.c timer.c stream.c frame.c  \
			synth.c decoder.c layer12.c layer3.c huffman.c  \
			$(headers) $(data_includes)

EXTRA_libmad_la_SOURCES =	imdct_l_arm.S #synth_mmx.S

libmad_la_DEPENDENCIES =	@ASO_OBJS@
libmad_la_LIBADD =		@ASO_OBJS@

INCLUDES =		$(FPM) $(ASO)

BUILT_SOURCES =		mad.h
CLEANFILES =		mad.h

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

version_current =	2
version_revision =	1
version_age =		2

version_info =		$(version_current):$(version_revision):$(version_age)

libmad_la_LDFLAGS =	-version-info $(version_info)

mad.h: config.status config.h Makefile.am  \
		$(srcdir)/COPYRIGHT $(srcdir)/mad.h.sed $(exported_headers)
	(sed -e '1s|.*|/*|' -e '1b' -e '$$s|.*| */|' -e '$$b'  \
		-e 's/^.*/ *&/' $(srcdir)/COPYRIGHT; echo;  \
	echo "# ifdef __cplusplus";  \
	echo 'extern "C" {';  \
	echo "# endif"; echo;  \
	if [ ".$(FPM)" != "." ]; then  \
		echo ".$(FPM)" | sed -e 's|^\.-D|# define |'; echo;  \
	fi;  \
	sed -ne 's/^# *define  *\(HAVE_.*_ASM\).*/# define \1/p'  \
		config.h; echo;  \
	sed -ne 's/^# *define  *OPT_\(SPEED\|ACCURACY\).*/# define OPT_\1/p'  \
		config.h; echo;  \
	sed -ne 's/^# *define  *\(SIZEOF_.*\)/# define \1/p'  \
		config.h; echo;  \
	for header in $(exported_headers); do  \
		echo;  \
		sed -n -f $(srcdir)/mad.h.sed $(srcdir)/$$header;  \
	done; echo;  \
	echo "# ifdef __cplusplus";  \
	echo '}';  \
	echo "# endif") >$@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

.c.s:
	$(COMPILE) -S $<

again:
	$(MAKE) clean
	$(MAKE)

.PHONY: again
