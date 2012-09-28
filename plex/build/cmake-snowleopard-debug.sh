#!/bin/sh

# Everything is setup to compile with CLang
export CC=gcc-4.2
export CXX=g++-4.2
export PATH=../Dependencies/bin:$PATH
cmake -DOSX_SDK=/Developer/SDKs/MacOSX10.6.sdk -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=output ../..
