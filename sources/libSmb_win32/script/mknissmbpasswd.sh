#!/bin/sh
#
# Copyright (C) 1998 Benny Holmgren
#
# Script to import smbpasswd file into the smbpasswd NIS+ table. Reads
# from stdin the smbpasswd file.
#
while true
do
  read row
  if [ -z "$row" ]
  then
    break
  fi

  if [ "`echo $row | cut -c1`" = "#" ]
  then
    continue
  fi

  nistbladm -a \
    name=\"`echo $row | cut -d: -f1`\" \
    uid=\"`echo $row | cut -d: -f2`\" \
    lmpwd=\"`echo $row | cut -d: -f3`\" \
    ntpwd=\"`echo $row | cut -d: -f4`\" \
    acb=\"`echo $row | cut -d: -f5`\" \
    pwdlset_t=\"`echo $row | cut -d: -f6`\" \
    gcos=\"`echo $row | cut -d: -f7`\" \
    home=\"`echo $row | cut -d: -f8`\" \
    shell=\"`echo $row | cut -d: -f9`\"  smbpasswd.org_dir.`nisdefaults -d`
done
