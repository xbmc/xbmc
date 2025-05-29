#!/bin/sh
cd ~/kodi/build/
make -j$(getconf _NPROCESSORS_ONLN)
make apk
cd ~/kodi