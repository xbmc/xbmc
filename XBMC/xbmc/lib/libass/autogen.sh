#!/bin/sh
echo Running autoreconf
autoreconf --install
echo Running configure...
./configure $@
