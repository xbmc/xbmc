#!/bin/bash

if [ -d "$XBMC_DEPENDS_ROOT/buildtools-native/bin" ]; then
  PATH=$PATH:$XBMC_DEPENDS_ROOT/buildtools-native/bin
else
  PATH=$PATH:$XBMC_DEPENDS_ROOT/toolchain/bin
fi

if [ "$ACTION" = build ] ; then
  make -f ${SRCROOT}/codegenerator.mk
  make -f ${SRCROOT}/xbmc/gen-compileinfo.mk
fi
