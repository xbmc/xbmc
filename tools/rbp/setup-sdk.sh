#!/bin/bash

SCRIPT_PATH=$(cd `dirname $0` && pwd)

USE_BUILDROOT=1

if [ "$USE_BUILDROOT" = "1" ]; then
  BUILDROOT=${BUILDROOT:-"/opt/xbmc-bcm/buildroot"}
  TARBALLS=${TARBALLS:-"/opt/xbmc-tarballs"}
  XBMCPREFIX=${XBMCPREFIX:-"/opt/xbmc-bcm/xbmc-bin"}

  SDKSTAGE=$BUILDROOT/output/staging
  TARGETFS=$BUILDROOT/output/target
  TOOLCHAIN=$BUILDROOT/output/host/usr/
else
  TARBALLS=/opt/xbmc-tarballs
  SDKSTAGE=/opt/bcm-rootfs
  XBMCPREFIX=/opt/xbmc-bcm/xbmc-bin
  TARGETFS=/opt/bcm-rootfs
  TOOLCHAIN=/usr/local/bcm-gcc
  BUILDROOT=/opt/bcm-rootfs
fi

if [ -d $XBMCPREFIX ]
then
  [ -O $XBMCPREFIX ] || { command -v sudo >/dev/null 2>&1 && SUDO="sudo"; }
else
  [ -w $(dirname $XBMCPREFIX) ] || { command -v sudo >/dev/null 2>&1 && SUDO="sudo"; }
fi

if [ -z "$KODI_MIRROR" ]
then
  KODI_MIRROR="http://mirrors.kodi.tv"
fi

$SUDO mkdir -p $XBMCPREFIX
$SUDO chmod 777 $XBMCPREFIX
mkdir -p $XBMCPREFIX/lib
mkdir -p $XBMCPREFIX/include

echo "SDKSTAGE=$SDKSTAGE"                                              >  $SCRIPT_PATH/Makefile.include
echo "XBMCPREFIX=$XBMCPREFIX"                                          >> $SCRIPT_PATH/Makefile.include
echo "TARGETFS=$TARGETFS"                                              >> $SCRIPT_PATH/Makefile.include
echo "TOOLCHAIN=$TOOLCHAIN"                                            >> $SCRIPT_PATH/Makefile.include
echo "BUILDROOT=$BUILDROOT"                                            >> $SCRIPT_PATH/Makefile.include
echo "USE_BUILDROOT=$USE_BUILDROOT"                                    >> $SCRIPT_PATH/Makefile.include
echo "BASE_URL=${KODI_MIRROR}/build-deps/darwin-libs"                  >> $SCRIPT_PATH/Makefile.include
echo "TARBALLS_LOCATION=$TARBALLS"                                     >> $SCRIPT_PATH/Makefile.include
echo "RETRIEVE_TOOL=/usr/bin/curl"                                     >> $SCRIPT_PATH/Makefile.include
echo "RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output \$(TARBALLS_LOCATION)/\$(ARCHIVE)" >> $SCRIPT_PATH/Makefile.include
echo "ARCHIVE_TOOL=/bin/tar"                                           >> $SCRIPT_PATH/Makefile.include
echo "ARCHIVE_TOOL_FLAGS=xf"                                           >> $SCRIPT_PATH/Makefile.include
echo "JOBS=$((`grep -c processor /proc/cpuinfo -1`))"                  >> $SCRIPT_PATH/Makefile.include
