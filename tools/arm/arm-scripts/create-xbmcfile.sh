#!/bin/sh

# A re-write of original XBMC Makefile install proceedure that will work with scratchbox.

filename=xbmc.tar.bz2
prefix=tools/arm/arm-scripts/usr
cd ../../../
mkdir -p $prefix

# Install Binaries
echo "Copying XBMC binary to $prefix/share/xbmc/xbmc.bin"
install -D xbmc.bin $prefix/share/xbmc/xbmc.bin
install -D xbmc-xrandr $prefix/share/xbmc/xbmc-xrandr
# Install Scripts
install -D tools/Linux/xbmc.sh $prefix/bin/xbmc
install -D tools/Linux/xbmc-standalone.sh $prefix/bin/xbmc-standalone
install -D -m 0644 tools/Linux/FEH-ARM.py $prefix/share/xbmc/FEH.py
install -D -m 0644 tools/Linux/xbmc-xsession.desktop $prefix/share/xsessions/XBMC.desktop
# Arch dependent files
find system screensavers visualisations -type f -not -iregex ".*\(svn.*\|win32\(dx\)?\.vis\|osx\.vis\)" -iregex ".*\(arm.*\|\.vis\|\.xbs\)" -exec install -D "{}" $prefix/share/xbmc/"{}" \; -printf " -- %-75.75f\r"
# Install Datas
echo "Copying support and legal files..."
for FILE in `ls README.linux README.armel LICENSE.GPL *.txt`
do
  install -D -m 0644 "$FILE" $prefix/share/xbmc/"$FILE"
done
echo "Done!"
echo "Copying system files to $prefix/share/xbmc"
# Arch independent files
find language media scripts sounds userdata visualisations system -type f -not -iregex ".*\(svn.*\|\.so\|\.dll\|\.pyd\|python/.*\.zlib\|\.vis\)" -exec install -D -m 0644 "{}" $prefix/share/xbmc/"{}" \; -printf " -- %-75.75f\r"
# Skins
find skin -type f -not -iregex ".*\(svn.*\|^skin/[^/]*/media/.*[^x][^b][^t]\)" -exec install -D -m 0644 '{}' $prefix/share/xbmc/'{}' \; -printf " -- %-75.75f\r"
# Icons and links
mkdir -p $prefix/share/applications $prefix/share/pixmaps
cp -a tools/Linux/xbmc.png $prefix/share/pixmaps/
cp -a tools/Linux/xbmc.desktop $prefix/share/applications/
# Install Web
mkdir -p $prefix/share/xbmc/web
cp -r web/Project_Mayhem_III/* $prefix/share/xbmc/web
find $prefix/share/xbmc/web -depth -name .svn -exec rm -rf {} \;
echo "...Complete!"

cd arm-scripts

# Cleanup
if [ -e $filename ]
then
  rm $filename
fi

echo "Creating tar file... please wait"
tar cjf $filename usr
rm -r usr
echo "Done! Output: $filename"
