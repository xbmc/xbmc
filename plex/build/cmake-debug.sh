#!/bin/sh

# Everything is setup to compile with CLang
export CC=clang
export CXX=clang
export PATH=/usr/local/Cellar/ccache/3.1.8/libexec/:$PATH
cmake -GNinja -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=output ../..
