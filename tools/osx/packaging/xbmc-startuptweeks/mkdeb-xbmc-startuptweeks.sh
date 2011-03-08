#!/bin/sh

if [ -f "/usr/bin/sudo" ]; then
  SUDO="/usr/bin/sudo"
fi

PACKAGE=com.xbmc.xbmc-startuptweeks

VERSION=1.0
REVISION=0
ARCHIVE=${PACKAGE}_${VERSION}-${REVISION}_iphoneos-arm.deb

echo Creating $PACKAGE package version $VERSION revision $REVISION
${SUDO} rm -rf $PACKAGE
${SUDO} rm -rf $ARCHIVE

# create debian control file
mkdir -p $PACKAGE/DEBIAN
echo "Package: $PACKAGE"                          >  $PACKAGE/DEBIAN/control
echo "Priority: Extra"                            >> $PACKAGE/DEBIAN/control
echo "Name: XBMC startup tweeks (seatbelt)"       >> $PACKAGE/DEBIAN/control
echo "Version: $VERSION-$REVISION"                >> $PACKAGE/DEBIAN/control
echo "Architecture: iphoneos-arm"                 >> $PACKAGE/DEBIAN/control
echo "Description: XBMC startup tweeks, removes seatbelt" >> $PACKAGE/DEBIAN/control
echo "Homepage: http://xbmc.org/"                 >> $PACKAGE/DEBIAN/control
echo "Maintainer: Scott Davilla"                  >> $PACKAGE/DEBIAN/control
echo "Author: TeamXBMC"                           >> $PACKAGE/DEBIAN/control
echo "Section: Multimedia"                        >> $PACKAGE/DEBIAN/control

# prerm: called on remove and upgrade - get rid of existing bits 
echo "#!/bin/sh"                                  >  $PACKAGE/DEBIAN/prerm
echo "rm -f /usr/libexec/xbmc/startup"            >> $PACKAGE/DEBIAN/prerm
echo "rm -f /System/Library/LaunchDaemons/com.xbmc.xbmc.startup.plist" >> $PACKAGE/DEBIAN/prerm
chmod +x $PACKAGE/DEBIAN/prerm

# postinst: startup our daemon plist
echo "#!/bin/sh"                                  >  $PACKAGE/DEBIAN/postinst
echo "/bin/launchctl load /System/Library/LaunchDaemons/com.xbmc.xbmc.startup.plist 2&> /dev/null" >> $PACKAGE/DEBIAN/postinst
chmod +x $PACKAGE/DEBIAN/postinst

# create ios launch daemon that runs at boot
DEST=System/Library/LaunchDaemons
mkdir -p $PACKAGE/$DEST
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" >  $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "<plist version=\"1.0\">"                    >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "<dict>"                                     >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "  <key>Label</key>"                         >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "  <string>com.xbmc.xbmc.startup</string>"   >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "  <key>Program</key>"                       >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "  <string>/usr/libexec/xbmc/startup</string>" >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "  <key>RunAtLoad</key>"                     >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "  <true/>"                                  >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "</dict>"                                    >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
echo "</plist>"                                   >> $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist
${SUDO} chmod 644 $PACKAGE/$DEST/com.xbmc.xbmc.startup.plist

# create startup file that is run by our launch daemon
DEST=usr/libexec/xbmc
mkdir -p $PACKAGE/$DEST
echo "#!/bin/sh"                                  >  $PACKAGE/$DEST/startup
echo "#remove sandbox/seatbelt restrictions"      >> $PACKAGE/$DEST/startup
echo "if echo \`sysctl hw.machine\` | grep AppleTV2,1 > /dev/null; then"    >> $PACKAGE/$DEST/startup
echo "  sysctl -w security.mac.proc_enforce=0"    >> $PACKAGE/$DEST/startup
echo "fi"                                         >> $PACKAGE/$DEST/startup
echo "sysctl -w security.mac.vnode_enforce=0"     >> $PACKAGE/$DEST/startup
echo "exit 0"                                     >> $PACKAGE/$DEST/startup
${SUDO} chmod 755 $PACKAGE/$DEST/startup

# set ownership to root:root
${SUDO} chown -R 0:0 $PACKAGE

echo Packaging $PACKAGE
# Tell tar, pax, etc. on Mac OS X 10.4+ not to archive
# extended attributes (e.g. resource forks) to ._* archive members.
# Also allows archiving and extracting actual ._* files.
export COPYFILE_DISABLE=true
export COPY_EXTENDED_ATTRIBUTES_DISABLE=true
../../ios-depends/build/bin/dpkg-deb -b $PACKAGE $ARCHIVE
../../ios-depends/build/bin/dpkg-deb --info $ARCHIVE
../../ios-depends/build/bin/dpkg-deb --contents $ARCHIVE

# clean up by removing package dir
${SUDO} rm -rf $PACKAGE
