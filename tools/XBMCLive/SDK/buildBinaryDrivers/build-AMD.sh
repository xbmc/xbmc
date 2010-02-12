#!/bin/bash

THISDIR=$(pwd)

. $THISDIR/getInstallers.sh
 
mkdir -p $THISDIR/Files/chroot_local-includes/root &> /dev/null
 
if ! ls $THISDIR/ati*.run > /dev/null 2>&1 ; then
	getAMDInstaller
else
	mv $THISDIR/ati*.run $THISDIR/Files/chroot_local-includes/root
fi
