#!/bin/bash

#
# Needed packages: build-essential cdbs fakeroot dh-make debhelper debconf libstdc++5 dkms
#

if [ ! -f /etc/mtab ]; then
	ln -s /proc/mounts /etc/mtab
fi

cd /root

sh ./ati-driver-*.run --buildpkg Ubuntu/karmic

mkdir Files
dpkg-deb -x fglrx-amdcccle_*.deb Files
dpkg-deb -x fglrx-kernel-source_*.deb Files
dpkg-deb -x fglrx-modaliases_*.deb Files
dpkg-deb -x libamdxvba1_*.deb Files
dpkg-deb -x xorg-driver-fglrx_*.deb Files
dpkg-deb -x xorg-driver-fglrx-*.deb Files

cd ./Files

# Assuming only one kernel is installed!
modulesdir=/lib/modules/$(ls /lib/modules)

kernelVersion=$(basename $modulesdir)
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
depmod -a $kernelVersion
tar cvf /tmp/modules.tar modules.* updates
rm updates/dkms/fglrx.ko
popd

pushd .
mkdir -p lib/modules/$kernelVersion
cd lib/modules/$kernelVersion
tar xvf /tmp/modules.tar
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

