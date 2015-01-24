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

if [ ! -f /etc/mtab ]; then
	ln -s /proc/mounts /etc/mtab
fi

cd /root

if ! ls ./NVIDIA-Linux-*.run > /dev/null 2>&1; then
	exit
fi

sh ./NVIDIA-Linux-*.run --extract-only

cd NVIDIA-Linux-*

mv $(ls -1d * | grep -v usr) usr/bin

pushd .
cd usr/lib
ln -s libcuda.so.* libcuda.so.1
ln -s libGLcore.so.* libGLcore.so.1
ln -s libGL.so.* libGL.so.1
ln -s libnvidia-cfg.so.* libnvidia-cfg.so.1
ln -s libnvidia-tls.so.* libnvidia-tls.so.1
ln -s vdpau/libvdpau_nvidia.so.* libvdpau_nvidia.so.1
ln -s libvdpau.so.* libvdpau.so.1
ln -s vdpau/libvdpau_trace.so.* libvdpau_trace.so.1
ln -s libcuda.so.1 libcuda.so
ln -s libGLcore.so.1 libGLcore.so
ln -s libGL.so.1 libGL.so
ln -s libnvidia-cfg.so.1 libnvidia-cfg.so
ln -s libnvidia-tls.so.1 libnvidia-tls.so
ln -s libvdpau_nvidia.so.1 libvdpau_nvidia.so
ln -s libvdpau.so.1 libvdpau.so
ln -s libvdpau_trace.so.1 libvdpau_trace.so
popd
pushd .
cd usr/lib/tls
ln -s libnvidia-tls.so.* libnvidia-tls.so.1
ln -s libnvidia-tls.so.1 libnvidia-tls.so
popd
pushd .
cd usr
mkdir lib/xorg
mv X11R6/lib/* lib/xorg
cd lib/xorg
ln -s libXvMCNVIDIA.so.* libXvMCNVIDIA.so.1
ln -s libXvMCNVIDIA.so.1 libXvMCNVIDIA.so
cd modules
ln -s libnvidia-wfb.so.* libnvidia-wfb.so.1
ln -s libnvidia-wfb.so.1 libnvidia-wfb.so
cd extensions
ln -s libglx.so.* libglx.so.1
ln -s libglx.so.1 libglx.so
popd

# Assuming only one kernel is installed!
kernelVersion=$(ls /lib/modules)
modulesdir=/lib/modules/$kernelVersion

apt-get install linux-headers-$kernelVersion

pushd .
cd usr/src/nv
make SYSSRC=/usr/src/linux-headers-$kernelVersion/ module
cp nvidia.ko /tmp
rm *.o *.ko
popd

pushd .
cd $modulesdir
mkdir -p updates/dkms

cp /tmp/nvidia.ko updates/dkms
depmod $kernelVersion
tar cf /tmp/modules.tar modules.* updates
rm updates/dkms/nvidia.ko
popd

pushd .
mkdir -p lib/modules/$kernelVersion
cd lib/modules/$kernelVersion
tar xf /tmp/modules.tar
rm /tmp/modules.tar
popd

overhead=10
IMAGE_SIZE=$(($(du -sm . | cut -f1) + $overhead))

dd if=/dev/zero of=/tmp/nvidia.ext3 bs=1M count=$IMAGE_SIZE
mkfs.ext3 -F /tmp/nvidia.ext3

mkdir ../Image
mount -o loop /tmp/nvidia.ext3 ../Image
cp -RP * ../Image
umount ../Image
rm -rf ../Image

cd /root
rm -rf NVIDIA-Linux-*
