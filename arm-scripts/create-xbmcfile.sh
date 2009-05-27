#!/bin/sh

# A re-write of original XBMC Makefile install proceedure that will work with scratchbox.

prefix=../arm-scripts/tmp
cd ../XBMC


echo "Creating target directories in $(prefix)/share/xbmc"
find language media screensavers scripts skin sounds userdata visualisations system -type d -not -iregex ".*svn.*" -exec mkdir -p $prefix/share/xbmc/"{}" \; -printf " -- %f                            \r"
echo "Copying system files to $prefix/share/xbmc"
find language media screensavers scripts sounds userdata visualisations system -type f -not -iregex ".*\(svn.*\|\.so\|\.dll\|win32\.vis\|osx\.vis\)" -exec cp "{}" $prefix/share/xbmc/"{}" \; -printf " -- %f                            \r"
find system -type f -not -iregex ".*svn.*" -iregex ".*arm.*" -exec cp "{}" $prefix/share/xbmc/"{}" \; -printf " -- %f                            \r"
find skin -type f -not -iregex ".*\(svn.*\|\.png|\.gif\)" -exec cp '{}' $prefix/share/xbmc/'{}' \; -printf " -- %f                            \r"


mkdir -p $prefix/share/xbmc/web
unzip -oq web/Project_Mayhem_III_webserver_v1.0.zip -d $prefix/share/xbmc/web


echo "Copying XBMC binary to $prefix/share/xbmc/xbmc.bin"
cp -f xbmc.bin $prefix/share/xbmc/xbmc.bin
mkdir -p $prefix/bin
cp tools/Linux/xbmc.sh $prefix/bin/xbmc
cp tools/Linux/xbmc-standalone.sh $prefix/bin/xbmc-standalone
cp tools/Linux/FEH.py $prefix/share/xbmc/FEH.py
mkdir -p $prefix/share/xsessions
cp tools/Linux/xbmc-xsession.desktop /usr/share/xsessions/XBMC.desktop
chmod 755 $prefix/bin/xbmc
chmod 755 $prefix/bin/xbmc-standalone
echo "Copying support and legal files..."
cp README.linux README.armel LICENSE.GPL *.txt xbmc-xrandr $prefix/share/xbmc/
echo "Done!"


cd $prefix/../

# Cleanup
if [ -e xbmc.tar.bz2 ]; then
	rm xbmc.tar.bz2
fi

echo "Creating tar file... please wait"
cd tmp
tar cjf ../xbmc.tar.bz2 ./
cd ../
rm -r tmp
echo "Done! Output: xbmc.tar.bz2"
