#!/bin/bash

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

# $1 is cmdline path
# $2 is bootstub path
# $3 is bzImage path
# $4 is initrd path
# $5 is rootfs path
# $6 is nand image size in MB
# $7 is boot image (content of logical partition1, DEBUG for ESL)
# $8 is output nand image (logical partition1 +logical partition2)

if [ $# -lt 8 ]; then
	echo "usage: nand.sh cmdline_path bootstub_path bzImage_path initrd_path rootfs_path img_size mem_size boot_path img_path"
	exit 1
fi

if [ ! -e "$1" ]; then
	echo "cmdline file not exist!"
	exit 1
fi

if [ ! -e "$2" ]; then
	echo "bootstub file not exist!"
	exit 1
fi

if [ ! -e "$3" ]; then
	echo "no kernel bzImage file!"
	exit 1
fi

if [ ! -e "$4" ]; then
	echo "no initrd file!"
	exit 1
fi

if [ ! -d "$5" ]; then
	echo "no rootfs path!"
	exit 1
fi

# convert a decimal number to the sequence that printf could recognize to output binary integer (not ASCII)
binstring ()
{
	h1=$(($1%256))
	h2=$((($1/256)%256))
	h3=$((($1/256/256)%256))
	h4=$((($1/256/256/256)%256))
	binstr=`printf "\x5cx%02x\x5cx%02x\x5cx%02x\x5cx%02x" $h1 $h2 $h3 $h4`
}

# add cmdline to the first part of boot image
cat $1 /dev/zero | dd of=$7 bs=4096 count=1

# append bootstub
cat $2 /dev/zero | dd of=$7 bs=4096 count=1 seek=1

# append bzImage and initrd
cat $3 $4 | dd of=$7 bs=4096 seek=2

# fill bzImage_size and initrd_size
binstring `stat -c %s $3`
printf $binstr | dd of=$7 bs=1 seek=256 conv=notrunc
binstring `stat -c %s $4`
printf $binstr | dd of=$7 bs=1 seek=260 conv=notrunc

# quick test by exiting here, only get *.bin.boot file for ESL
echo 'I will exit here to only produce [name].bin.boot file for ESL testing'
exit 0

# prepare the final image contains two logical partitions
HEADERS=4
SECTORS=16

dd if=/dev/zero of=$8 bs=$((1024*1024)) count=$6
size=`stat -c %s $7`
units=${HEADERS}*${SECTORS}*512

# partition1_cylinders actually contain the partition table's 1 track of sectors
partition1_cylinders=$(((size + units-1 + SECTORS*512)/units))
echo "logical partition1 is $partition1_cylinders"
/sbin/fdisk -C $(($6*1024*1024/units)) -H ${HEADERS} -S ${SECTORS} $8 <<EOF
n
p
1

$partition1_cylinders
n
p
2


w
EOF

# ok, is time to slice the disk to pieces :-)
# first, it the logical partition table
dd if=$8 bs=512 count=${SECTORS} of=$8-0
# then the logical partition1
dd if=$8 bs=512 count=$((partition1_cylinders*HEADERS*SECTORS-SECTORS)) skip=${SECTORS} of=$8-1
# then all the rest are for partition2
dd if=$8 bs=512 skip=$((partition1_cylinders*HEADERS*SECTORS)) of=$8-2

# prepare the logical partition1
dd if=$7 of=$8-1 bs=4096 conv=notrunc
# prepare the logical partition2, format it to ext3 and copy rootfs onto it
/sbin/mkfs -t ext3 $8-2 <<EOF
y
EOF
sudo mkdir -p /tmp/$(basename $8)-2.mnt
sudo mount -o loop $8-2 /tmp/$(basename $8)-2.mnt
echo "cp -a $5/* /tmp/$(basename $8)-2.mnt"
sudo cp -av $5/* /tmp/$(basename $8)-2.mnt
sudo umount $8-2
sudo rm -rf /tmp/$(basename $8)-2.mnt

# ok, is time to combine the slices
cat $8-0 $8-1 $8-2 | dd of=$8 bs=4096
rm -rf $8-0 $8-1 $8-2

# done
echo "done with creating NAND image : whole image file (boot partition + rootfs partition)is $8, the boot image file is $7 (for ESL)"
