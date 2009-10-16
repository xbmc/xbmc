#!/bin/sh

# A re-write of original XBMC Makefile install proceedure that will work with scratchbox.

filename=xbmc.tar.bz2
prefix=arm-scripts/usr/local
cd ../
mkdir -p $prefix


echo "Copying XBMC binary to $prefix/share/xbmc/xbmc.bin"
install -D xbmc.bin $prefix/share/xbmc/xbmc.bin
install -D xbmc-xrandr $prefix/share/xbmc/xbmc-xrandr
install -D tools/Linux/xbmc.sh $prefix/bin/xbmc
install -D tools/Linux/xbmc-standalone.sh $prefix/bin/xbmc-standalone
install -D -m 0644 tools/Linux/FEH-ARM.py $prefix/share/xbmc/FEH.py
install -D -m 0644 tools/Linux/xbmc-xsession.desktop $prefix/share/xsessions/XBMC.desktop
echo "Copying support and legal files..."
for FILE in `ls README.* LICENSE.GPL *.txt`
do
  install -D -m 0644 "$FILE" $prefix/share/xbmc/"$FILE"
done

echo "Copying system files to $prefix/share/xbmc"
# Arch independent files
find language media scripts sounds userdata visualisations system -type f -not -iregex ".*\(svn.*\|\.so\|\.dll\|\.pyd\|python/.*\.zlib\|\.vis\)" -exec install -D -m 0644 "{}" $prefix/share/xbmc/"{}" \; -printf " -- %-75.75f\r"
# Arch dependent files
find system screensavers visualisations -type f -not -iregex ".*\(svn.*\|win32\.vis\|osx\.vis\)" -iregex ".*\(arm.*\|\.vis\|\.xbs\)" -exec install -D "{}" $prefix/share/xbmc/"{}" \; -printf " -- %-75.75f\r"
# Skins
find skin -type f -not -iregex ".*\(svn.*\|\.png\|\.gif\)" -exec install -D -m 0644 '{}' $prefix/share/xbmc/'{}' \; -printf " -- %-75.75f\r"

echo "Copying web files to $prefix/share/xbmc/web"
mkdir -p $prefix/share/xbmc/web
cp -r web/Project_Mayhem_III/* $prefix/share/xbmc/web
find $prefix/share/xbmc/web -depth -name .svn -exec rm -rf {} \;
echo "Done!"

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
