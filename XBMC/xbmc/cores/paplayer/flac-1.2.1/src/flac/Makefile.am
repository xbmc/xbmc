#  flac - Command-line FLAC encoder/decoder
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

bin_PROGRAMS = flac

AM_CFLAGS = @OGG_CFLAGS@

EXTRA_DIST = \
	Makefile.lite \
	Makefile.lite.iffscan \
	flac.dsp \
	flac.vcproj \
	iffscan.c \
	iffscan.dsp \
	iffscan.vcproj

flac_SOURCES = \
	analyze.c \
	decode.c \
	encode.c \
	foreign_metadata.c \
	main.c \
	local_string_utils.c \
	utils.c \
	vorbiscomment.c \
	analyze.h \
	decode.h \
	encode.h \
	foreign_metadata.h \
	local_string_utils.h \
	utils.h \
	vorbiscomment.h

flac_LDADD = \
	$(top_builddir)/src/share/grabbag/libgrabbag.la \
	$(top_builddir)/src/share/getopt/libgetopt.a \
	$(top_builddir)/src/share/replaygain_analysis/libreplaygain_analysis.la \
	$(top_builddir)/src/share/replaygain_synthesis/libreplaygain_synthesis.la \
	$(top_builddir)/src/share/utf8/libutf8.la \
	$(top_builddir)/src/libFLAC/libFLAC.la \
	@OGG_LIBS@ \
	@LIBICONV@ \
	@MINGW_WINSOCK_LIBS@ \
	-lm
