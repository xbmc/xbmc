#!/bin/bash
# A run-script for ubrowser which sets up the right runtime paths etc.

# Tweakables here.
ARCH=`arch -m`-linux

####

OLDPWD=`pwd`
(cd ../../libraries/${ARCH}/runtime_release/ && LD_LIBRARY_PATH=`pwd`/:${LD_LIBRARY_PATH} ${OLDPWD}/ubrowser $*)

####
