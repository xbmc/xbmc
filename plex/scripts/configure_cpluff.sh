#!/bin/sh
DEPENDPATH="$1/bin"
CPLUFFDIR=$2

export CFLAGS="-arch i386"
export LDFLAGS=$CFLAGS
export PATH=$DEPENDPATH:$PATH
$CPLUFFDIR/configure --disable-nls
