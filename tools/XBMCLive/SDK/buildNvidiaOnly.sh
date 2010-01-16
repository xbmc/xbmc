#!/bin/sh

export SDK_BUILDHOOKS="./nvidiaOnlyHook.sh"
export DONOTBUILDRESTRICTEDDRIVERS=1

./build.sh
