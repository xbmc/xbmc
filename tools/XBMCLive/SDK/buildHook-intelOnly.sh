#!/bin/sh

# Remove unneeded files
rm $WORKPATH/buildLive/Files/binary_local-includes/live/*.module

export DONOTBUILDRESTRICTEDDRIVERS=1

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
