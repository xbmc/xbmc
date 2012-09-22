#!/bin/sh

# Everything is setup to compile with CLang
export CC=clang
export CXX=clang
cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=output ../..
