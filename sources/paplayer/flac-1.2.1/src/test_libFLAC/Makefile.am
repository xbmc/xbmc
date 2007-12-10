#  test_libFLAC - Unit tester for libFLAC
#  Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

EXTRA_DIST = \
	Makefile.lite \
	test_libFLAC.dsp \
	test_libFLAC.vcproj

INCLUDES = -I$(top_srcdir)/src/libFLAC/include

noinst_PROGRAMS = test_libFLAC
test_libFLAC_LDADD = \
	$(top_builddir)/src/share/grabbag/libgrabbag.la \
	$(top_builddir)/src/share/replaygain_analysis/libreplaygain_analysis.la \
	$(top_builddir)/src/test_libs_common/libtest_libs_common.la \
	$(top_builddir)/src/libFLAC/libFLAC.la \
	@OGG_LIBS@ \
	@MINGW_WINSOCK_LIBS@ \
	-lm
test_libFLAC_SOURCES = \
	bitwriter.c \
	decoders.c \
	encoders.c \
	format.c \
	main.c \
	metadata.c \
	metadata_manip.c \
	metadata_object.c \
	bitwriter.h \
	decoders.h \
	encoders.h \
	format.h \
	metadata.h
