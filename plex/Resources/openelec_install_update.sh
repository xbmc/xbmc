#!/bin/sh

# we need to exit with error if something goes wrong
set -e

UPDATEFILE=$1
#INSTALLPATH=/storage/.update
INSTALLPATH=/tmp/update

# first we make sure to create the update path:
if [ ! -d $INSTALLPATH ]; do
	mkdir -p $INSTALLPATH
fi

tar -xf -C $INSTALLPATH --strip-components 2 $UPDATEFILE */target/*
