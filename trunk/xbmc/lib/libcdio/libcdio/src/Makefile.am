#   $Id: Makefile.am,v 1.44 2008/03/15 16:21:28 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2005, 2006, 2008
#    Rocky Bernstein <rocky@gnu.org>
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

GETOPT_C = getopt.c getopt1.c

man_MANS = cd-drive.1 cd-info.1 cd-read.1 iso-read.1 iso-info.1 
EXTRA_DIST = cd-drive.help2man cd-info.help2man cd-read.help2man \
	     iso-info.help2man iso-read.help2man $(GETOPT_C) getopt.h \
             $(man_MANS)

noinst_HEADERS = cddb.h getopt.h util.h

####################################################
# Things to make the utility/diagnostic programs
####################################################

if BUILD_CD_PARANOIA
SUBDIRS =  cd-paranoia
endif

CDDB_LIBS=@CDDB_LIBS@
CDDA_PLAYER_LIBS=@CDDA_PLAYER_LIBS@

$(man_MANS): %.1: % %.help2man
	-$(HELP2MAN) --opt-include=$<.help2man --no-info --output=$@ ./$<
MOSTLYCLEANFILES = $(man_MANS)

if BUILD_CDDA_PLAYER
cdda_player_SOURCES = cdda-player.c cddb.c cddb.h $(GETOPT_C)
cdda_player_LDADD   = $(LIBCDIO_LIBS) $(CDDB_LIBS) $(CDDA_PLAYER_LIBS)
bin_cdda_player     = cdda-player
endif

if BUILD_CD_DRIVE
cd_drive_SOURCES = cd-drive.c util.c util.h $(GETOPT_C)
cd_drive_LDADD   =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
bin_cd_drive     = cd-drive
endif

if BUILD_CDINFO
cd_info_SOURCES = cd-info.c cddb.c cddb.h util.c util.h $(GETOPT_C)
cd_info_LDADD   =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(CDDB_LIBS) $(VCDINFO_LIBS) $(LIBICONV)
bin_cd_info     = cd-info 
endif

if BUILD_CDINFO_LINUX
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD   = $(LIBCDIO_LIBS)
bin_cdinfo_linux     = cdinfo-linux
endif

if BUILD_CD_READ
cd_read_SOURCES = cd-read.c util.c util.h $(GETOPT_C)
cd_read_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
bin_cd_read     = cd-read 
endif

if BUILD_ISO_INFO
iso_info_SOURCES = iso-info.c util.c util.h $(GETOPT_C)
iso_info_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
bin_iso_info     = iso-info
endif

if BUILD_ISO_READ
iso_read_SOURCES = iso-read.c util.c util.h $(GETOPT_C)
iso_read_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
bin_iso_read     = iso-read
endif

mmc_tool_SOURCES = mmc-tool.c util.c util.h $(GETOPT_C)
mmc_tool_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
bin_mmc_tool     = mmc-tool

bin_PROGRAMS = $(bin_cd_drive) $(bin_cd_info)  $(bin_cdinfo_linux) $(bin_cd_read) $(bin_iso_info) $(bin_iso_read) $(bin_cdda_player) $(bin_mmc_tool)

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS) $(VCDINFO_CFLAGS) $(CDDB_CFLAGS)
