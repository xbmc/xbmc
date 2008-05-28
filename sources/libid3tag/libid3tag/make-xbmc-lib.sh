#!/bin/bash 

g++ -shared -o libid3tag-i486-linux.so .libs/*.o `cat ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.def` ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.o -fPIC -O3
