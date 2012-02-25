#!/bin/sh

# usage: ./mkdeb-xbmc-ios.sh release/debug (case insensitive)
# Allows us to run mkdeb-xbmc-ios.sh from anywhere in the three, rather than the tools/darwin/packaging/xbmc-ios folder only
SWITCH=`echo $1 | tr [A-Z] [a-z]`
DIRNAME=`dirname $0`
DSYM_TARGET_DIR=/Users/Shared/xbmc-depends/dSyms
DSYM_FILENAME=XBMC.app.dSYM

if [ ${SWITCH:-""} = "debug" ]; then
  echo "Packaging Debug target for iOS"
  XBMC="$DIRNAME/../../../../build/Debug-iphoneos/XBMC.app"
  DSYM="$DIRNAME/../../../../build/Debug-iphoneos/$DSYM_FILENAME"  
elif [ ${SWITCH:-""} = "release" ]; then
  echo "Packaging Release target for iOS"
  XBMC="$DIRNAME/../../../../build/Release-iphoneos/XBMC.app"
  DSYM="$DIRNAME/../../../../build/Release-iphoneos/$DSYM_FILENAME"   
else
  echo "You need to specify the build target"
  exit 1 
fi  

#copy bzip2 of dsym to xbmc-depends install dir
if [ -d $DSYM ]; then
  if [ -d $DSYM_TARGET_DIR ]; then
    tar -C $DSYM/.. -c $DSYM_FILENAME/ | bzip2 > $DSYM_TARGET_DIR/`$DIRNAME/../../../buildbot/gitrev-posix`-${DSYM_FILENAME}.tar.bz2
  fi
fi


if [ ! -d $XBMC ]; then
  echo "XBMC.app not found! are you sure you built $1 target?"
  exit 1
fi
if [ -f "/usr/libexec/fauxsu/libfauxsu.dylib" ]; then
  export DYLD_INSERT_LIBRARIES=/usr/libexec/fauxsu/libfauxsu.dylib
elif [ -f "/usr/bin/sudo" ]; then
  SUDO="/usr/bin/sudo"
fi
if [ -f "/Users/Shared/xbmc-depends/toolchain/bin/dpkg-deb" ]; then
  # make sure we pickup our tar, gnutar will fail when dpkg -i
  bin_path=$(cd /Users/Shared/xbmc-depends/toolchain/bin; pwd)
  export PATH=${bin_path}:${PATH}
fi

PACKAGE=org.xbmc.xbmc-ios

VERSION=11.0                                                                                                                                                                                                    
REVISION=0~rc1
ARCHIVE=${PACKAGE}_${VERSION}-${REVISION}_iphoneos-arm.deb

echo Creating $PACKAGE package version $VERSION revision $REVISION
${SUDO} rm -rf $DIRNAME/$PACKAGE
${SUDO} rm -rf $DIRNAME/$ARCHIVE

# create debian control file.
mkdir -p $DIRNAME/$PACKAGE/DEBIAN
echo "Package: $PACKAGE"                          >  $DIRNAME/$PACKAGE/DEBIAN/control
echo "Priority: Extra"                            >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Name: XBMC-iOS"                             >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Depends: firmware (>= 4.1), curl, org.xbmc.xbmc-iconpack" >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Version: $VERSION-$REVISION"                >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Architecture: iphoneos-arm"                 >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Description: XBMC Multimedia Center for 4.x iOS" >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Homepage: http://xbmc.org/"                 >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Maintainer: Scott Davilla, Edgar Hucek"     >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Author: TeamXBMC"                           >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Section: Multimedia"                        >> $DIRNAME/$PACKAGE/DEBIAN/control
echo "Icon: file:///Applications/Cydia.app/Sources/mirrors.xbmc.org.png" >> $DIRNAME/$PACKAGE/DEBIAN/control

# prerm: called on remove and upgrade - get rid of existing bits.
echo "#!/bin/sh"                                  >  $DIRNAME/$PACKAGE/DEBIAN/prerm
echo "find /Applications/XBMC.app -delete"        >> $DIRNAME/$PACKAGE/DEBIAN/prerm
chmod +x $DIRNAME/$PACKAGE/DEBIAN/prerm

# postinst: nothing for now.
echo "#!/bin/sh"                                  >  $DIRNAME/$PACKAGE/DEBIAN/postinst
echo "chown -R mobile:mobile /Applications/XBMC.app" >> $DIRNAME/$PACKAGE/DEBIAN/postinst
chmod +x $DIRNAME/$PACKAGE/DEBIAN/postinst

# prep XBMC.app
mkdir -p $DIRNAME/$PACKAGE/Applications
cp -r $XBMC $DIRNAME/$PACKAGE/Applications/
find $DIRNAME/$PACKAGE/Applications/ -name '.svn' -exec rm -rf {} \;
find $DIRNAME/$PACKAGE/Applications/ -name '.gitignore' -exec rm -rf {} \;
find $DIRNAME/$PACKAGE/Applications/ -name '.DS_Store'  -exec rm -rf {} \;

# set ownership to root:root
${SUDO} chown -R 0:0 $DIRNAME/$PACKAGE

echo Packaging $PACKAGE
# Tell tar, pax, etc. on Mac OS X 10.4+ not to archive
# extended attributes (e.g. resource forks) to ._* archive members.
# Also allows archiving and extracting actual ._* files.
export COPYFILE_DISABLE=true
export COPY_EXTENDED_ATTRIBUTES_DISABLE=true
#
dpkg-deb -b $DIRNAME/$PACKAGE $DIRNAME/$ARCHIVE
dpkg-deb --info $DIRNAME/$ARCHIVE
dpkg-deb --contents $DIRNAME/$ARCHIVE

# clean up by removing package dir
${SUDO} rm -rf $DIRNAME/$PACKAGE
