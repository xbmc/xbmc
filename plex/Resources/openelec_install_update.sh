#!/bin/sh

# we need to exit with error if something goes wrong
set -e

UPDATEFILE=$1
INSTALLPATH=/storage/.update

# first we make sure to create the update path:
if [ ! -d $INSTALLPATH ]; then
	mkdir -p $INSTALLPATH
fi

# Grab KERNEL and SYSTEM path within tarball
KERNEL=$(tar -tf $UPDATEFILE | grep KERNEL$)
SYSTEM=$(tar -tf $UPDATEFILE | grep SYSTEM$)
KERNELMD5=$(tar -tf $UPDATEFILE | grep KERNEL.md5)
SYSTEMMD5=$(tar -tf $UPDATEFILE | grep SYSTEM.md5)

# untar both SYSTEM and KERNEL into installation directory
tar -xf $UPDATEFILE -C $INSTALLPATH  $KERNEL $SYSTEM $KERNELMD5 $SYSTEMMD5

# move extracted files to the toplevel
cd $INSTALLPATH
mv $KERNEL $SYSTEM $KERNELMD5 $SYSTEMMD5 .

# remove the directories created by tar
rm -r */
rm $UPDATEFILE
