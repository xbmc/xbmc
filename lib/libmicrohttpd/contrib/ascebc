#!/bin/sh
iconv -f UTF-8 -t IBM-1047 $1 > temp.file
if test -x $1
then 
  rm $1
  mv temp.file $1
  chmod +x $1
else
  rm $1
  mv temp.file $1
fi
