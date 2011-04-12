#!/bin/bash

if [ -f xbmc_asap.dll ]
then
make clean
fi

make xbmc

cp xbmc_asap.dll /xbmc/system/players/paplayer/