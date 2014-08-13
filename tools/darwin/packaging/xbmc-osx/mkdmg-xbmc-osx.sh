#!/bin/sh

# usage: ./mkdmg-xbmc-osx.sh release/debug (case insensitive)
# Allows us to run mkdmg-xbmc-osx.sh from anywhere in the three, rather than the tools/darwin/packaging/xbmc-osx folder only
SWITCH=`echo $1 | tr [A-Z] [a-z]`
DIRNAME=`dirname $0`

if [ ${SWITCH:-""} = "debug" ]; then
  echo "Packaging Debug target for OSX"
  XBMC="$DIRNAME/../../../../build/Debug/XBMC.app"
elif [ ${SWITCH:-""} = "release" ]; then
  echo "Packaging Release target for OSX"
  XBMC="$DIRNAME/../../../../build/Release/XBMC.app"
else
  echo "You need to specify the build target"
  exit 1 
fi

if [ ! -d $XBMC ]; then
  echo "XBMC.app not found! are you sure you built $1 target?"
  exit 1
fi
ARCHITECTURE=`file $XBMC/Contents/MacOS/XBMC | awk '{print $NF}'`

PACKAGE=org.xbmc.xbmc-osx

VERSION=13.2
REVISION=0
ARCHIVE=${PACKAGE}_${VERSION}-${REVISION}_macosx-intel-${ARCHITECTURE}

echo Creating $PACKAGE package version $VERSION revision $REVISION
${SUDO} rm -rf $DIRNAME/$ARCHIVE

if [ -e "/Volumes/$ARCHIVE" ]
then 
  umount /Volumes/$ARCHIVE
fi

$DIRNAME/dmgmaker.pl $XBMC $ARCHIVE

echo "done"
