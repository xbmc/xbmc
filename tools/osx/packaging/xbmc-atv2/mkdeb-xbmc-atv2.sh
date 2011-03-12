#!/bin/sh
# usage: ./mkdeb-xbmc-atv2.sh release/debug (case insensitive)

SWITCH=`echo $1 | tr [A-Z] [a-z]`

if [ $SWITCH = "debug" ]; then
  echo "Packaging Debug target for ATV2"
  XBMC="../../../../build/Debug-iphoneos/XBMC.frappliance"
elif [ $SWITCH = "release" ]; then
  echo "Packaging Release target for ATV2"
  XBMC="../../../../build/Release-iphoneos/XBMC.frappliance"
else
  echo "You need to specify the build target"
  exit 1 
fi 

if [ ! -d $XBMC ]; then
  echo "XBMC.frappliance not found! are you sure you built $1 target?"
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

PACKAGE=org.xbmc.xbmc-atv2

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
echo "Name: XBMC-ATV2"                            >> $PACKAGE/DEBIAN/control
echo "Depends: curl, org.awkwardtv.whitelist, org.xbmc.xbmc-seatbeltunlock" >> $PACKAGE/DEBIAN/control
echo "Version: $VERSION-$REVISION"                >> $PACKAGE/DEBIAN/control
echo "Architecture: iphoneos-arm"                 >> $PACKAGE/DEBIAN/control
echo "Description: XBMC Multimedia Center for AppleTV 2" >> $PACKAGE/DEBIAN/control
echo "Homepage: http://xbmc.org/"                 >> $PACKAGE/DEBIAN/control
echo "Maintainer: Scott Davilla, Edgar Hucek"     >> $PACKAGE/DEBIAN/control
echo "Author: TeamXBMC"                           >> $PACKAGE/DEBIAN/control
echo "Section: Multimedia"                        >> $PACKAGE/DEBIAN/control

# prerm: called on remove and upgrade - get rid of existing bits.
echo "#!/bin/sh"                                  >  $PACKAGE/DEBIAN/prerm
echo "rm -rf /Applications/XBMC.frappliance"      >> $PACKAGE/DEBIAN/prerm
echo "if [ \"\`uname -r\`\" = \"10.3.1\" ]; then" >> $PACKAGE/DEBIAN/prerm
echo "  rm -rf /Applications/Lowtide.app/Appliances/XBMC.frappliance" >> $PACKAGE/DEBIAN/prerm
echo "else"                                       >> $PACKAGE/DEBIAN/prerm
echo "  rm -rf /Applications/AppleTV.app/Appliances/XBMC.frappliance" >> $PACKAGE/DEBIAN/prerm
echo "fi"                                         >> $PACKAGE/DEBIAN/prerm
chmod +x $PACKAGE/DEBIAN/prerm

# postinst: symlink XBMC.frappliance into correct location and reload Lowtide/AppleTV.
echo "#!/bin/sh"                                  >  $PACKAGE/DEBIAN/postinst
echo "if [ \"\`uname -r\`\" = \"10.3.1\" ]; then" >> $PACKAGE/DEBIAN/postinst
echo "  ln -sf /Applications/XBMC.frappliance /Applications/Lowtide.app/Appliances/XBMC.frappliance" >> $PACKAGE/DEBIAN/postinst
echo "  killall Lowtide"                          >> $PACKAGE/DEBIAN/postinst
echo "else"                                       >> $PACKAGE/DEBIAN/postinst
echo "  ln -sf /Applications/XBMC.frappliance /Applications/AppleTV.app/Appliances/XBMC.frappliance" >> $PACKAGE/DEBIAN/postinst
echo "  killall AppleTV"                          >> $PACKAGE/DEBIAN/postinst
echo "fi"                                         >> $PACKAGE/DEBIAN/postinst
chmod +x $PACKAGE/DEBIAN/postinst

# prep XBMC.frappliance
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
