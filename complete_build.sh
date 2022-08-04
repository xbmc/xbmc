#!/usr/bin/bash

# Modify build_config.sh to put your build tree where you like and
# to tweak compiler options. Currently I have been using clang-14

# Download icu-cmake, but I was not able to wire it in properly
./geticu.sh >/tmp/geticu.log 2>&1

./download_icu.sh >/tmp/download_icu.log 2>&1

# Will run sudo

./build_icu.sh >/tmp/build_icu.log 2>&1

# Will run sudo

./build.sh >/tmp/build.log 2>&1

./run_kodi.sh
