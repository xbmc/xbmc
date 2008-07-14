#  example_cpp_decode_file - Simple FLAC file decoder using libFLAC
#  Copyright (C) 2007  Josh Coalson
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
	example_cpp_decode_file.dsp \
	example_cpp_decode_file.vcproj

noinst_PROGRAMS = example_cpp_decode_file
example_cpp_decode_file_LDADD = \
	$(top_builddir)/src/libFLAC++/libFLAC++.la \
	$(top_builddir)/src/libFLAC/libFLAC.la \
	@OGG_LIBS@ \
	@MINGW_WINSOCK_LIBS@ \
	-lm
example_cpp_decode_file_SOURCES = main.cpp
