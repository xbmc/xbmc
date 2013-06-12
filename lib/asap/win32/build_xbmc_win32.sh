#!/bin/bash

MAKEFLAGS=""
BGPROCESSFILE="$2"

if [ "$1" == "clean" ]
then
make clean
fi
if [ $NUMBER_OF_PROCESSORS > 1 ]; then
  MAKEFLAGS=-j$NUMBER_OF_PROCESSORS
fi

make $MAKEFLAGS xbmc

cp xbmc_asap.dll /xbmc/system/players/paplayer/
#remove the bgprocessfile for signaling the process end
rm $BGPROCESSFILE
