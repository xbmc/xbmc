#!/bin/bash

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

#
if [ ! -f /etc/mtab ]; then
	ln -s /proc/mounts /etc/mtab
fi

cd /root
pushd .

if [ ! -f crystalhd-HEAD.tar.gz ]; then
	exit
fi

tar xf crystalhd-HEAD.tar.gz

# Make libraries
cd crystalhd/linux_lib/libcrystalhd
make

# Output is 
# lrwxrwxrwx libcrystalhd.so -> libcrystalhd.so.1.0
# lrwxrwxrwx libcrystalhd.so.1 -> libcrystalhd.so.1.0
# -rwxr-xr-x libcrystalhd.so.1.0

popd 

mkdir -p ./Files/lib/firmware
cp crystalhd/firmware/fwbin/70012/*.bin ./Files/lib/firmware

mkdir -p ./Files/usr/lib
mv crystalhd/linux_lib/libcrystalhd/libcrystalhd.so* ./Files/usr/lib

pushd .

# Assuming only one kernel is installed!
kernelVersion=$(ls /lib/modules)
modulesdir=/lib/modules/$kernelVersion

apt-get -y install linux-headers-$kernelVersion

# Make kernel module
cd crystalhd/driver/linux
autoconf
./configure --with-kernel-path=$modulesdir/build
make

# Output is 
# -rw-r--r-- crystalhd.ko

cp crystalhd.ko /tmp
popd

pushd .
cd $modulesdir
mkdir -p kernel/drivers/video/broadcom

cp /tmp/crystalhd.ko kernel/drivers/video/broadcom
depmod $kernelVersion
tar cf /tmp/modules.tar modules.* kernel/drivers/video/broadcom/*
popd

pushd .
mkdir -p ./Files/etc/udev/rules.d/
cp -f crystalhd/driver/linux/20-crystalhd.rules ./Files/etc/udev/rules.d/
mkdir -p ./Files/lib/udev/rules.d/
cp -f crystalhd/driver/linux/20-crystalhd.rules ./Files/lib/udev/rules.d/

mkdir -p ./Files/lib/modules/$kernelVersion
cd ./Files/lib/modules/$kernelVersion
tar xf /tmp/modules.tar
rm /tmp/modules.tar
popd

pushd .
# Prepare tar for real build
cd Files
tar cf /tmp/crystalhd.tar *

popd
rm -rf ./Files
