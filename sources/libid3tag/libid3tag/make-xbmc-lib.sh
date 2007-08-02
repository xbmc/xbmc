#!/bin/bash 

ld -o libid3tag-i486-linux.so --export-dynamic -shared  --whole-archive .libs/libid3tag.a `grep __wrap ../../../XBMC/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk -F"__wrap_"  '{print $2}' | awk -F"(" '{out=out"--wrap "$1" "} END {print out}'` 
