#   $Id: Makefile.am,v 1.42 2008/03/15 17:26:13 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2005, 2006
#    Rocky Bernstein <rocky@cpan.org>
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
# Sample programs
####################################################
#
if ENABLE_CPP
SUBDIRS =  C++
endif
if BUILD_CD_PARANOIA
paranoia_progs = paranoia paranoia2
endif
if BUILD_EXAMPLES
noinst_PROGRAMS = audio cdchange cdtext device drives eject \
	          isofile isofile2 isofuzzy isolist isolsn \
	          mmc1 mmc2 mmc2a mmc3 $(paranoia_progs) tracks \
	          sample3 sample4 udf1 udffile cdio-eject
endif

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

audio_DEPENDENCIES    = $(LIBCDIO_DEPS)
audio_LDADD           = $(LIBCDIO_LIBS) $(LIBICONV)

cdchange_DEPENDENCIES = $(LIBCDIO_DEPS)
cdchange_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)

cdtext_DEPENDENCIES   = $(LIBCDIO_DEPS)
cdtext_LDADD          = $(LIBCDIO_LIBS) $(LIBICONV)

device_DEPENDENCIES   = $(LIBCDIO_DEPS)
device_LDADD          = $(LIBCDIO_LIBS) $(LIBICONV)

drives_DEPENDENCIES   = $(LIBCDIO_DEPS)
drives_LDADD          = $(LIBCDIO_LIBS) $(LIBICONV)

eject_DEPENDENCIES    = $(LIBCDIO_DEPS)
eject_LDADD           = $(LIBCDIO_LIBS) $(LIBICONV)

cdio_eject_DEPENDENCIES = $(LIBCDIO_DEPS)
cdio_eject_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)

if BUILD_CD_PARANOIA
paranoia_LDADD  = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
paranoia2_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
endif

isofile_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isofile2_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isofuzzy_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isolist_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isolsn_LDADD     = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

mmc1_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc1_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)
mmc2_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc2_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)

mmc2a_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc2a_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)

mmc3_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc3_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)

sample3_DEPENDENCIES = $(LIBCDIO_DEPS)
sample3_LDADD = $(LIBCDIO_LIBS) $(LIBICONV)

sample4_DEPENDENCIES = $(LIBCDIO_DEPS)
sample4_LDADD = $(LIBCDIO_LIBS) $(LIBICONV)

tracks_DEPENDENCIES = $(LIBCDIO_DEPS)
tracks_LDADD        = $(LIBCDIO_LIBS) $(LIBICONV)

udf1_DEPENDENCIES = $(LIBUDF_LIBS) $(LIBCDIO_DEPS)
udf1_LDADD        = $(LIBUDF_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

udffile_DEPENDENCIES = $(LIBUDF_LIBS) $(LIBCDIO_DEPS)
udffile_LDADD        = $(LIBUDF_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)


# iso programs create file "copying"
MOSTLYCLEANFILES = copying
