#!/bin/bash

THISDIR=$(pwd)

. $THISDIR/getInstallers.sh
 
mkdir -p $THISDIR/Files/chroot_local-includes/root &> /dev/null

# Get drivers if files are not already available
if [ ! -f $THISDIR/crystalhd-HEAD.tar.gz ]; then
	getBCDriversSources
else
	mv $THISDIR/crystalhd-HEAD.tar.gz $THISDIR/Files/chroot_local-includes/root
fi
