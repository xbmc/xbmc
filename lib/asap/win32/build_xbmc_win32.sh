#!/bin/bash

if [ "$1" == "clean" ]
then
make clean
fi

make xbmc

cp xbmc_asap.dll /xbmc/system/players/paplayer/