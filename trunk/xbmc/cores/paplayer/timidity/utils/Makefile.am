# TiMidity++ -- MIDI to WAVE converter and player
# Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
# Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

include $(top_srcdir)/common.makefile.in

INCLUDES = \
	-I$(top_srcdir)/libarc \
	-I$(top_srcdir)/timidity

noinst_LIBRARIES = libutils.a

libutils_a_SOURCES = \
	bitset.c \
	bitset.h \
	fft4g.c \
	fft4g.h \
	tmdy_getopt.h \
	mblock.c \
	mblock.h \
	memb.c \
	memb.h \
	nkflib.c \
	nkflib.h \
	strtab.c \
	strtab.h \
	support.c \
	support.h \
	timer.c \
	timer.h

EXTRA_libutils_a_SOURCES = \
	fft.c \
	fft.h \
	getopt.c \
	support.c \
	net.c \
	net.h \
	mac_util.c \
	mac_util.h \
	mac_readdir.c \
	mac_readdir.h \
	readdir.h \
	readdir_win.c

if NEEDGETOPT
GETOPT_OBJS = getopt.$(OBJEXT)
endif

if ENABLE_NETWORK
NET_OBJS = net.$(OBJEXT)
endif

if ENABLE_SOUND_SPEC
SOUND_SPEC_OBJS = fft.$(OBJEXT)
endif

if W32READDIR
W32READDIR_OBJS = readdir_win.$(OBJEXT)
endif

libutils_LIBADD= \
	$(NET_OBJS) \
	$(W32READDIR_OBJS) \
	$(SOUND_SPEC_OBJS) \
	$(GETOPT_OBJS)

libutils_a_LIBADD=$(libutils_LIBADD)

libutils.a: $(libutils_a_OBJECTS) $(libutils_a_DEPENDENCIES)
if VCPP
	rm -f libutils.a
	rm -f utils.lib
	link -lib $(libutils_a_OBJECTS) $(libutils_LIBADD) -out:utils.lib
	if test -f utils.lib ; then touch $@ ; fi
else
if BORLANDC
	rm -f libutils.a
	rm -f utils.lib
	for foo in  $(libutils_a_OBJECTS) $(libutils_a_LIBADD);do \
	  tlib utils.lib +$$foo; \
	done
	if test -f utils.lib ; then touch $@ ; fi
else
if WATCOM_C
	rm -f libutils.a
	rm -f utils.lib
	for foo in  $(libutils_a_OBJECTS) $(libutils_a_LIBADD);do \
	wlib -q utils.lib +$$foo; \
	done
	if test -f utils.lib ; then touch $@ ; fi
else
	-rm -f libutils.a
	$(libutils_a_AR) libutils.a $(libutils_a_OBJECTS) $(libutils_a_LIBADD)
	$(RANLIB) libutils.a
endif
endif
endif

clean:
	rm -f *.$(OBJEXT)
	rm -f *.$(so)
	rm -f *.a
	rm -f *.lib
	
