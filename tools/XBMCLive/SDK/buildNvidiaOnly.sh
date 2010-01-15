#!/bin/sh

export SDK_BUILDHOOK=nvidiaOnlyHook.sh
export DONOTBUILDRESTRICTEDDRIVERS=1

./build.sh
