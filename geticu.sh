#!/usr/bin/bash
. ./build_config.sh
set -x
#
# Downloads the icu-cmake source. 
#
# I have not been able to use icu-cmake. It provides a way to download and 
# configure icu. It is also supposed to simplify integrating into a cmake
# build, buth that is where I get lost. It does NOT build icu for you,
# it makes it possible so that it gets built along with your project.

mkdir -p "${KODI_SOURCE}"/tools/depends/target/libicu/
cd  "${KODI_SOURCE}"/tools/depends/target/libicu/


curl -L https://github.com/viaduck/icu-cmake/archive/master.tar.gz | tar zxv


