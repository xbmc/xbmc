#!/bin/sh

cd ~/kodi/tools/depends
./bootstrap
./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=arm-linux-androideabi --with-sdk-path=$HOME/android-tools/android-sdk-linux --prefix=$HOME/android-tools/xbmc-depends
make -j$(getconf _NPROCESSORS_ONLN)