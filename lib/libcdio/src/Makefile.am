#   $Id: Makefile.am,v 1.31 2005/01/06 11:55:09 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
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
####################################################
# Things to make the sample/test programs
####################################################

if BUILD_CD_PARANOIA
SUBDIRS =  cd-paranoia
endif

CDDB_LIBS=@CDDB_LIBS@

if MAINTAINER_MODE
MAINTAINERCLEANFILES = $(man_MANS)
man_MANS = cd-drive.1 cd-info.1 cd-read.1 iso-read.1 iso-info.1 
$(man_MANS): %.1: %
	-$(HELP2MAN) --output=$@ ./$<
endif

if BUILD_CD_DRIVE
cd_drive_SOURCES = cd-drive.c util.c util.h
cd_drive_LDADD   =  $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(LIBICONV)
bin_cd_drive     = cd-drive
endif

if BUILD_CDINFO
cd_info_SOURCES = cd-info.c util.c util.h
cd_info_LDADD   =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(CDDB_LIBS) $(VCDINFO_LIBS) $(LIBICONV)
bin_cd_info     = cd-info 
endif

if BUILD_CDINFO_LINUX
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD   = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)
bin_cdinfo_linux     = cdinfo-linux
endif

if BUILD_CD_READ
cd_read_SOURCES = cd-read.c util.c util.h
cd_read_LDADD   = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(LIBICONV)
bin_cd_read     = cd-read 
endif

if BUILD_ISO_INFO
iso_info_SOURCES = iso-info.c util.c util.h
iso_info_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(LIBICONV)
bin_iso_info     = iso-info
endif

if BUILD_ISO_READ
iso_read_SOURCES = iso-read.c util.c util.h
iso_read_LDADD   =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(LIBICONV)
bin_iso_read     = iso-read
endif

bin_PROGRAMS = $(bin_cd_drive) $(bin_cd_info)  $(bin_cdinfo_linux) $(bin_cd_read) $(bin_iso_info) $(bin_iso_read)

# Put LIBPOPT_CFLAGS after local include in case LIBPOPT has headers common
# to those in LIBCDIO_CFLAGS
INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS) $(VCDINFO_CFLAGS) $(CDDB_CFLAGS) $(LIBPOPT_CFLAGS) 
