#!/bin/sh

# usage: ./mkdeb-xbmc-ios.sh release/debug (case insensitive)

SWITCH=`echo $1 | tr [A-Z] [a-z]`

if [ $SWITCH = "debug" ]; then
  echo "Packaging Debug target for iOS"
  XBMC="../../../../build/Debug-iphoneos/XBMC.app"
elif [ $SWITCH = "release" ]; then
  echo "Packaging Release target for iOS"
  XBMC="../../../../build/Release-iphoneos/XBMC.app"
else
  echo "You need to specify the build target"
  exit 1 
fi  

if [ ! -d $XBMC ]; then
  echo "XBMC.app not found! are you sure you built $1 target?"
  exit 1
fi
if [ -f "/usr/bin/sudo" ]; then
  SUDO="/usr/bin/sudo"
fi
if [ -f "/Users/Shared/xbmc-depends/ios-4.2_arm7/bin/dpkg-deb" ]; then
  # make sure we pickup our tar, gnutar will fail when dpkg -i
  bin_path=$(cd /Users/Shared/xbmc-depends/ios-4.2_arm7/bin; pwd)
  export PATH=${bin_path}:${PATH}
fi

PACKAGE=org.xbmc.xbmc-ios

VERSION=10.0
REVISION=7
ARCHIVE=${PACKAGE}_${VERSION}-${REVISION}_iphoneos-arm.deb

echo Creating $PACKAGE package version $VERSION revision $REVISION
${SUDO} rm -rf $PACKAGE
${SUDO} rm -rf $ARCHIVE

# create debian control file.
mkdir -p $PACKAGE/DEBIAN
echo "Package: $PACKAGE"                          >  $PACKAGE/DEBIAN/control
echo "Priority: Extra"                            >> $PACKAGE/DEBIAN/control
echo "Name: XBMC-iOS"                             >> $PACKAGE/DEBIAN/control
echo "Depends: firmware (>= 4.1), curl, org.xbmc.xbmc-iconpack" >> $PACKAGE/DEBIAN/control
echo "Version: $VERSION-$REVISION"                >> $PACKAGE/DEBIAN/control
echo "Architecture: iphoneos-arm"                 >> $PACKAGE/DEBIAN/control
echo "Description: XBMC Multimedia Center for 4.x iOS" >> $PACKAGE/DEBIAN/control
echo "Homepage: http://xbmc.org/"                 >> $PACKAGE/DEBIAN/control
echo "Maintainer: Scott Davilla, Edgar Hucek"     >> $PACKAGE/DEBIAN/control
echo "Author: TeamXBMC"                           >> $PACKAGE/DEBIAN/control
echo "Section: Multimedia"                        >> $PACKAGE/DEBIAN/control
echo "Icon: file:///Applications/Cydia.app/Sources/mirrors.xbmc.org.png" >> $PACKAGE/DEBIAN/control

# prerm: called on remove and upgrade - get rid of existing bits.
echo "#!/bin/sh"                                  >  $PACKAGE/DEBIAN/prerm
echo "rm -rf /Applications/XBMC.app"              >> $PACKAGE/DEBIAN/prerm
chmod +x $PACKAGE/DEBIAN/prerm

# postinst: nothing for now.
echo "#!/bin/sh"                                  >  $PACKAGE/DEBIAN/postinst
chmod +x $PACKAGE/DEBIAN/postinst

# prep XBMC.app
mkdir -p $PACKAGE/Applications
cp -r $XBMC $PACKAGE/Applications/
find $PACKAGE/Applications/ -name '.svn' -exec rm -rf {} \;
find $PACKAGE/Applications/ -name '.gitignore' -exec rm -rf {} \;
find $PACKAGE/Applications/ -name '.DS_Store'  -exec rm -rf {} \;

# set ownership to root:root
${SUDO} chown -R 0:0 $PACKAGE

echo Packaging $PACKAGE
# Tell tar, pax, etc. on Mac OS X 10.4+ not to archive
# extended attributes (e.g. resource forks) to ._* archive members.
# Also allows archiving and extracting actual ._* files.
export COPYFILE_DISABLE=true
export COPY_EXTENDED_ATTRIBUTES_DISABLE=true
#
dpkg-deb -b $PACKAGE $ARCHIVE
dpkg-deb --info $ARCHIVE
dpkg-deb --contents $ARCHIVE

# clean up by removing package dir
${SUDO} rm -rf $PACKAGE
