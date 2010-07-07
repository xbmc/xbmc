#!/bin/sh

SRC_LIB_RTMP="/opt/local/lib/librtmp.so.0"
DST_LIB_RTMP="../../system/librtmp.so"

if [ -f $SRC_LIB_RTMP ]; then
  # copy librtmp into xbmc's system directory, we
  # rename it to librtmp.so and skip the symlinking.
  cp $SRC_LIB_RTMP $DST_LIB_RTMP

  # rename any dependency libs to inside xbmc's app framework
  for a in $(otool -L "$DST_LIB_RTMP" | grep opt | awk ' { print $1 } ') ; do 
    echo "Processing $a"
    install_name_tool -change "$a" @executable_path/../Frameworks/$(basename $a) "$DST_LIB_RTMP"
  done
fi
