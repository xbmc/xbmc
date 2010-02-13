#!/bin/sh

#      Copyright (C) 2005-2008 Team XBMC
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

# Remove copy restricted from installer helper
rm $WORKPATH/buildDEBs/xbmclive-installhelpers/finish-install.d/07copyRestricted

# Remove holding on the kernel
rm $WORKPATH/buildLive/Files/chroot_local-hooks/01-holdKernel

# Remove unneeded files
rm $WORKPATH/buildLive/Files/binary_local-includes/live/*.module

# Remove unneeded files
rm $WORKPATH/buildBinaryDrivers/build-NVIDIA.sh
rm $WORKPATH/buildBinaryDrivers/build-AMD.sh

rm $WORKPATH/copyFiles-restricted.sh


# Modify menu.lst
if [ -f $WORKPATH/buildLive/Files/binary_grub/menu.lst ]; then
	sed -i '/## BEGIN NVIDIA ##/,/## END NVIDIA ##/d' $WORKPATH/buildLive/Files/binary_grub/menu.lst
	sed -i '/## BEGIN AMD ##/,/## END AMD ##/d' $WORKPATH/buildLive/Files/binary_grub/menu.lst
fi

# Modify grub.cfg
if [ -f $WORKPATH/buildLive/Files/binary_grub/grub.cfg ]; then
	sed -i '/## BEGIN NVIDIA ##/,/## END NVIDIA ##/d' $WORKPATH/buildLive/Files/binary_grub/grub.cfg
	sed -i '/## BEGIN AMD ##/,/## END AMD ##/d' $WORKPATH/buildLive/Files/binary_grub/grub.cfg
fi

# Modify syslinux menu
if [ -f $WORKPATH/buildLive/Files/binary_syslinux/live.cfg ]; then
	sed -i '/## BEGIN NVIDIA ##/,/## END NVIDIA ##/d' $WORKPATH/buildLive/Files/binary_syslinux/live.cfg
	sed -i '/## BEGIN AMD ##/,/## END AMD ##/d' $WORKPATH/buildLive/Files/binary_syslinux/live.cfg
fi
