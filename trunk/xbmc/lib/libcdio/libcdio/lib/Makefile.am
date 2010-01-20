#   $Id: Makefile.am,v 1.69 2006/03/30 10:47:51 flameeyes Exp $
#
#    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
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
########################################################
# make all libraries
########################################################

if BUILD_CD_PARANOIA
paranoiadirs = cdda_interface paranoia
endif

if ENABLE_CXX_BINDINGS
cxxdirs = cdio++
endif

SUBDIRS = driver iso9660 udf $(paranoiadirs) $(cxxdirs)
