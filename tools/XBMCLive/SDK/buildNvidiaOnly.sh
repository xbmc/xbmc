#!/bin/sh

export SDK_BUILDHOOK=nvidiaOnlyHook.sh
export DONOTBUILDRESTRICTEDDRIVERS=1

# Use one of these two
# ./buildWithProxy.sh
./build.sh
