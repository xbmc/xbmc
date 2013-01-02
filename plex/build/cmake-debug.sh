#!/bin/sh

# Everything is setup to compile with CLang
export CC=clang
export CXX=clang
export PATH=/usr/local/Cellar/ccache/3.1.8/libexec/:$PATH
cmake -GNinja -DPLEX_SKIP_BUNDLING=1 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=output ../..
