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
# Needed packages: build-essential cdbs fakeroot dh-make debhelper debconf libstdc++5 dkms
#

distroCodename=$(cat /etc/lsb-release | grep CODENAME | cut -d= -f2)

if [ ! -f /etc/mtab ]; then
	ln -s /proc/mounts /etc/mtab
fi

cd /root

if ! ls ./ati-driver-*.run > /dev/null 2>&1; then
	exit
fi

sh ./ati-driver-*.run --buildpkg Ubuntu/$distroCodename

mkdir Files
dpkg-deb -x fglrx-amdcccle_*.deb Files
dpkg-deb -x fglrx-kernel-source_*.deb Files
dpkg-deb -x fglrx-modaliases_*.deb Files
dpkg-deb -x libamdxvba1_*.deb Files
dpkg-deb -x xorg-driver-fglrx_*.deb Files
dpkg-deb -x xorg-driver-fglrx-*.deb Files

cd ./Files

# Assuming only one kernel is installed!
kernelVersion=$(ls /lib/modules)
modulesdir=/lib/modules/$kernelVersion

apt-get -y install linux-headers-$kernelVersion

pushd .
cd usr/src/fglrx-*/
./make.sh --uname_r $kernelVersion
cp 2.6.x/fglrx.ko /tmp
cd 2.6.x
make clean
popd

pushd .
cd $modulesdir
mkdir -p updates/dkms

cp /tmp/fglrx.ko updates/dkms
depmod $kernelVersion
tar cf /tmp/modules.tar modules.* updates
rm updates/dkms/fglrx.ko
popd

pushd .
mkdir -p lib/modules/$kernelVersion
cd lib/modules/$kernelVersion
tar xf /tmp/modules.tar
rm /tmp/modules.tar
popd

overhead=10
IMAGE_SIZE=$(($(du -sm . | cut -f1) + $overhead))

dd if=/dev/zero of=/tmp/amd.ext3 bs=1M count=$IMAGE_SIZE
mkfs.ext3 /tmp/amd.ext3 -F

mkdir ../Image
mount -o loop /tmp/amd.ext3 ../Image
cp -RP * ../Image
umount ../Image
rm -rf ../Image

cd /root
rm -rf ./Files
