#!/bin/sh

sed -i -e "/bootloader/s/grub/grub2/" $WORKPATH/buildLive/mkConfig.sh

sed -i -e "s/grub/grub-pc/" $WORKPATH/buildLive/Files/chroot_local-packageslists/packages.list

