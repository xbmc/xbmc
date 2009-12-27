#   $Id: Makefile.am,v 1.15 2005/01/22 03:41:50 rocky Exp $
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
# Things to regression testing
####################################################
#
if DISABLE_CPP
noinst_PROGRAMS = cdtext drives iso1 iso2 iso3 scsi-mmc1 scsi-mmc2 \
		  paranoia paranoia2 tracks sample2 sample3 sample4 
else  
noinst_PROGRAMS = cdtext drives iso1 iso2 iso3 paranoia scsi-mmc1 scsi-mmc2 \
		  paranoia paranoia2 tracks sample2 sample3 sample4 \
	          iso1cpp iso2cpp iso3cpp
endif

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

cdtext_LDADD  = $(LIBCDIO_LIBS)

drives_LDADD = $(LIBCDIO_LIBS)

paranoia_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)
paranoia2_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)

iso1_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso2_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso3_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

if !DISABLE_CPP
iso1cpp_SOURCES  = iso1cpp.cpp
iso1cpp_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso2cpp_SOURCES  = iso2cpp.cpp
iso2cpp_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso3cpp_SOURCES  = iso3cpp.cpp
iso3cpp_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
endif

scsi_mmc1_LDADD = $(LIBCDIO_LIBS)
scsi_mmc2_LDADD = $(LIBCDIO_LIBS)

sample2_LDADD = $(LIBCDIO_LIBS)
sample3_LDADD = $(LIBCDIO_LIBS)
sample4_LDADD = $(LIBCDIO_LIBS)

tracks_LDADD  = $(LIBCDIO_LIBS)

# iso programs create file "copying"
MOSTLYCLEANFILES = copying
