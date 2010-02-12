#!/bin/bash

THISDIR=$(pwd)

. $THISDIR/getInstallers.sh
  
mkdir -p $THISDIR/Files/chroot_local-includes/root &> /dev/null

if ! ls $THISDIR/NVIDIA*.run > /dev/null 2>&1 ; then
	getNVIDIAInstaller
else
	mv $THISDIR/NVIDIA*.run $THISDIR/Files/chroot_local-includes/root
fi
