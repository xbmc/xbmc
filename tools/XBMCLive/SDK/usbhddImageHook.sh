#!/bin/sh

# Set the output to be an USBHDD disk image
sed -i -e "/binary-images/s/iso/usb-hdd/" $WORKPATH/buildLive/mkConfig.sh

# We have to use syslinux in this case
sed -i -e "/bootloader/s/grub/syslinux/" $WORKPATH/buildLive/mkConfig.sh
