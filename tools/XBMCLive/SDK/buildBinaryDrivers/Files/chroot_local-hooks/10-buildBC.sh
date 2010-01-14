#!/bin/bash

#
if [ ! -f /etc/mtab ]; then
	ln -s /proc/mounts /etc/mtab
fi

cd /root
pushd .

tar xvf crystalhd-HEAD.tar.gz

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
modulesdir=/lib/modules/$(ls /lib/modules)

kernelVersion=$(basename $modulesdir)
apt-get -y install linux-headers-$kernelVersion

# Make kernel module
cd crystalhd/driver/linux
autoconf
./configure
make

# Output is 
# -rw-r--r-- crystalhd.ko

cp crystalhd.ko /tmp
popd

pushd .
cd $modulesdir
mkdir -p kernel/drivers/video/broadcom

cp /tmp/crystalhd.ko kernel/drivers/video/broadcom
depmod -a $kernelVersion
tar cvf /tmp/modules.tar modules.* kernel/drivers/video/broadcom/*
popd

pushd .
mkdir -p ./Files/lib/modules/$kernelVersion
cd ./Files/lib/modules/$kernelVersion
tar xvf /tmp/modules.tar
rm /tmp/modules.tar
popd

# Prepare tar for real build
cd Files
tar cvf /tmp/crystalhd.tar *
