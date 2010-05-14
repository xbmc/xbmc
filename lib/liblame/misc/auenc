#!/bin/sh
# 
# auenc -- version 0.1
#
# A wrapper for lame to encode multiple files.  By default, a .wav
# extension is removed and replaced by .mp3 .
#
# (C) 1999 Gerhard Wesp <gwesp@cosy.sbg.ac.at> under the GPL.

# set the variables below according to your taste
LAME=lame
LAME_OPTS="-S -h -v -V 0 -b 256" # high quality, silent operation

if [ $# -lt 1 ] ; then
  exec 1>&2
  cat << _EOF_
usage: $0 [options] file...
options:
  -d --delete: delete original file after successful encoding
_EOF_
  exit 1
fi

unset DELETE
case "$1" in
  -d | --delete ) DELETE=1 ; shift ;;
esac

for f
do
  $LAME $LAME_OPTS "$f" `basename "$f" .wav`.mp3 || {
    exec 1>&2
    echo "encoding of $f failed, aborting..."
    exit 1
  }
  if [ -n "$DELETE" ] ; then
    rm -f "$f"
  fi
done
