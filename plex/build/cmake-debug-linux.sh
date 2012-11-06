#!/bin/sh

# Everything is setup to compile with CLang
export CC=gcc
export CXX=g++
export PATH=/usr/lib/ccache:$PATH
cmake -GNinja -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=output ../..
