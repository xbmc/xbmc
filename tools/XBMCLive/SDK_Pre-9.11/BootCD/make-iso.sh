#!/bin/sh

#      Copyright (C) 2009-2010 Team XBMC
#      http://www.xbmc.org
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XBMC; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html

# NAME: make-iso.sh

# Creates an iso9660 image with rockridge extensions from the contents
# of directory cdimage.

CDIMAGE=cdimage
if [ ! -d $CDIMAGE ];then
   echo "You need to be cd'd to the directory above 'cdimage'"
   exit
fi

ISO=XBMCLive.iso

mkisofs -R -l -b boot/grub/stage2_eltorito -no-emul-boot  -boot-load-size 4 -boot-info-table -o $ISO $CDIMAGE 
#  -V "XBMCLiveCD" -P "http://xbmc.org" -p "http://xbmc.org" 

echo "ISO generated in $ISO"
