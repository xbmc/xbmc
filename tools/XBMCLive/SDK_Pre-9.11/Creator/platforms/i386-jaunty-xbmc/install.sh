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

# This is the script that will be placed onto the USB flash drive, that will
# run on bootup to install the software onto the device.

# I want us to error out if we use an undefined variable, so we will catch errors.
set -u

# Read in our config file, if it exists
if [ -f /install.cfg ]
then
    echo "Sourcing ./install.cfg"
    . /install.cfg
else
    echo "ERROR: ./install.cfg not found!"
    echo "In script: $0 $*"
    sleep 10
    halt
fi

#################### usplash functions start ####################################
SPLASHWRITE=0
# Determine if we have usplash_write available
type usplash_write > /dev/null 2>&1 && SPLASHWRITE=1

# Disable usplash, since we want text mode
SPLASHWRITE=0

BOOTDEVICE=$1

# show the progress at status bar.
# $1 = 0-100
splash_progress(){
    splash_write "PROGRESS $1"
    return 0
}
# display the text no matter whether verbose is set or not
splash_display(){
    echo "$@"
    splash_write "TEXT-URGENT $@"
    return 0
}
# set the splash delay time
splash_delay(){
    splash_write "TIMEOUT $1"
    return 0
}
# call the usplash_write command, if enabled
splash_write(){
    if [ "${SPLASHWRITE}" -eq 1 ]
    then
        usplash_write "$@"
    else:
	echo "ussplash_write: $@"
    fi
}
####################### usplash functions end ###############################

splash_delay 200
splash_display 'INSTALL..........'

pre_scsi_disk_number=$( ls /sys/class/scsi_disk | wc -l)
found=no
# Find the install disk
while true; do
      for device in 'hda' 'hdb' 'sda' 'sdb'
      do
        echo "checking device: /dev/${device} for installation target"
        if [ -e /sys/block/${device}/removable ]; then
           if [ "$(cat /sys/block/${device}/removable)" = "0" ]; then
              if cat /proc/mounts | grep /dev/${device}
              then
                  continue
              else
                  found="yes"
                  splash_display "found harddisk at /dev/${device}"
                  break
              fi
           fi
         fi
      done
      if [ "$found" = "yes" ]; then
        break;
      fi
      /bin/sleep 5
      echo "Did not find an installation target device"
done
echo "will install to /dev/${device}"

blocks=`fdisk -s /dev/${device}`
cylinders=$((blocks*2/63/255))

splash_display "Deleting Partition Table on /dev/${device} ..."
splash_delay 200
dd if=/dev/zero of=/dev/${device} bs=512 count=2
sync
splash_progress 5
splash_delay 10

splash_display "Creating New Partiton Table on /dev/${device} ..."
splash_delay 200

fdisk /dev/${device} <<EOF
n
p
1

$((boot_partition_size*1000/8192))
n
p
2

$((cylinders-((swap_partition_size+fat32_partition_size)*1000/8192)))
a
1
w
EOF

if [ $swap_partition_size -ne 0 ]
then
   fdisk /dev/${device} <<EOF
n
p
3

$((cylinders-(fat32_partition_size*1000/8192)))
t
3
82
w
EOF
   if [ $fat32_partition_size -gt 0 ]
   then
      fdisk /dev/${device} <<EOF
n
p
4


t
4
c
w
EOF
   fi
else if  [ $fat32_partition_size -gt 0 ]
         then
         fdisk /dev/${device} <<EOF
n
p
3


t
3
c
w
EOF
    fi
fi

sync
splash_progress 10
splash_delay 10

splash_display "Formatting /dev/${device}1 w/ ext3..."
splash_delay 200
mkfs.ext3 /dev/${device}1
sync
splash_progress 20
splash_delay 10

splash_display "Formatting /dev/${device}2 w/ ext3..."
splash_delay 200
mkfs.ext3 /dev/${device}2
sync
splash_progress 60
splash_delay 10

if [ $swap_partition_size -ne 0 ]
then
    splash_display "Formatting /dev/${device}3 w/ swap..."
    splash_delay 1000
    mkswap /dev/${device}3
    if [ $fat32_partition_size -ne 0 ]
    then
       splash_display "Formatting /dev/${device}4 w/ vfat..."
       splash_delay 1000
       mkfs.vfat /dev/${device}4
    fi
else if [ $fat32_partition_size -ne 0 ]
     then
       splash_display "Formatting /dev/${device}3 w/ vfat..."
       splash_delay 1000
       mkfs.vfat /dev/${device}3
    fi
fi
sync
splash_progress 65
splash_delay 10

splash_display 'Mounting partitions...'
splash_delay 200
mkdir /tmp/boot
mount -o loop -t squashfs /tmp/install/bootfs.img /tmp/boot

mount /dev/${device}2 /mnt
mkdir /mnt/boot
mount /dev/${device}1 /mnt/boot
splash_progress 70
splash_delay 10

splash_display 'Copying system files onto hard disk drive...'
splash_delay 200
cp -av /tmp/boot /mnt

if [ "${use_squashfs}" -eq 1 ]
then
    echo "Copying squashfs filesystem into place..."
    cp -v /tmp/install/rootfs.img /mnt/boot
else
    echo "Setting up NON squashfs filesystem..."
    mkdir /tmp/root
    mount -o loop -t squashfs /tmp/install/rootfs.img /tmp/root
    splash_display 'Copying system ROOT onto hard disk drive...'
    cp -av /tmp/root/. /mnt
fi
/usr/sbin/grub-install --root-directory=/mnt /dev/${device}
splash_progress 90
splash_delay 10

splash_display 'Unmounting partitions...'
splash_delay 200

umount /mnt/boot
umount /mnt
umount /tmp/boot
umount /tmp/install

splash_progress 95
splash_delay 10
sleep 1
splash_delay 6000
splash_display "Install Successfully"
# need to call reboot --help and let file system cache hold it, since we will
# unplug USB disk soon, and after that, reboot command will not be accessible.
# The reason why reboot still works sometimes without this is the whole
# "rootfs.img" is cached when it is copied to HD. But when rootfs.img become
# bigger and bigger the whole "rootfs.img" will not be able to fully cached (we
# have found this issue when creating big installation)
reboot --help > /dev/null 2>&1


case $BOOTDEVICE in
usb)
    splash_display "Unplug USB Key, System Will Reboot Automatically"
    while [ $pre_scsi_disk_number = $(ls /sys/class/scsi_disk | wc -l) ]
    do
        sleep 1
    done
    ;;
cd)
    splash_display "Sysstem Will Reboot after 5 seconds"
    sleep 5
    ;;
esac

splash_progress 100
splash_delay 1

reboot -f
