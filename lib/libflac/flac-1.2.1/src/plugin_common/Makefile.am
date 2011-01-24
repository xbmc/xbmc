#  plugin_common - Routines common to several plugins
#  Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libplugin_common.la

noinst_HEADERS = \
	all.h \
	charset.h \
	defs.h \
	dither.h \
	replaygain.h \
	tags.h

libplugin_common_la_SOURCES = \
	charset.c \
	dither.c \
	replaygain.c \
	tags.c

EXTRA_DIST = \
	Makefile.lite \
	README \
	plugin_common_static.dsp \
	plugin_common_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
