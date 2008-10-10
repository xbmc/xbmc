#!/bin/sh

# NAME: make-iso.sh

# Creates an iso9660 image with rockridge extensions from the contents
# of directory cdimage.

CDIMAGE=cdimage
if [ ! -d $CDIMAGE ];then
   echo "You need to be cd'd to the directory above 'cdimage'"
   exit
fi

ISO=XBMCLive.iso

mkisofs -o $ISO -b isolinux.bin -c boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table -l  -R -r $CDIMAGE

echo "ISO generated in $ISO"
