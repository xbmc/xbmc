#!/bin/sh

# A re-write of original XBMC Makefile install proceedure that will work with scratchbox.

prefix=../arm-scripts/tmp
cd ../XBMC


echo "Copying system files to $prefix/share/xbmc"
# Arch independent files
find language media scripts sounds userdata visualisations system -type f -not -iregex ".*\(svn.*\|\.so\|\.dll\|\.pyd\|python/.*\.zlib\|\.vis\)" -exec install -D -m 0644 "{}" $(prefix)/share/xbmc/"{}" \; -printf " -- %f                           \r"
# Arch dependent files
find system screensavers visualisations -type f -not -iregex ".*\(svn.*\|win32\.vis\|osx\.vis\)" -iregex ".*\(arm.*\|\.vis\|\.xbs\)" -exec install -D "{}" $(prefix)/share/xbmc/"{}" \; -printf " -- %f                           \r"
# Skins
find skin -type f -not -iregex ".*\(svn.*\|\.png\|\.gif\)" -exec install -D -m 0644 '{}' $(prefix)/share/xbmc/'{}' \; -printf " -- %f                            \r"

mkdir -p $(prefix)/share/xbmc/web
unzip -oq web/Project_Mayhem_III_webserver_v1.0.zip -d $(prefix)/share/xbmc/web -x "*/Thumbs.db"

echo "Copying XBMC binary to $(prefix)/share/xbmc/xbmc.bin"
install -D xbmc.bin $(prefix)/share/xbmc/xbmc.bin
install -D xbmc-xrandr $(prefix)/share/xbmc/xbmc-xrandr
install -D tools/Linux/xbmc.sh $(prefix)/bin/xbmc
install -D tools/Linux/xbmc-standalone.sh $(prefix)/bin/xbmc-standalone
install -D -m 0644 tools/Linux/FEH.py $(prefix)/share/xbmc/FEH.py
install -D -m 0644 tools/Linux/xbmc-xsession.desktop $(prefix)/share/xsessions/XBMC.desktop
echo "Copying support and legal files,,,"
for FILE in `ls README.linux LICENSE.GPL *.txt`; do install -D -m 0644 "$$FILE" $(prefix)/share/xbmc/; done
echo "Done!"
echo "You can run XBMC with the command 'xbmc'"


# OLDER VERSION!!! IGNORE!!!
#echo "Creating target directories in $prefix/share/xbmc"
#find language media screensavers scripts skin sounds userdata visualisations system -type d -not -iregex ".*svn.*" -exec mkdir -p $prefix/share/xbmc/"{}" \; -#printf " -- %f                            \r"
#echo "Copying system files to $prefix/share/xbmc"
#find language media screensavers scripts sounds userdata visualisations system -type f -not -iregex ".*\(svn.*\|\.so\|\.dll\|win32\.vis\|osx\.vis\)" -exec cp #"{}" $prefix/share/xbmc/"{}" \; -printf " -- %f                            \r"
#find system -type f -not -iregex ".*svn.*" -iregex ".*arm.*" -exec cp "{}" $prefix/share/xbmc/"{}" \; -printf " -- %f                            \r"
#find skin -type f -not -iregex ".*\(svn.*\|\.png|\.gif\)" -exec cp '{}' $prefix/share/xbmc/'{}' \; -printf " -- %f                            \r"
#
#
#mkdir -p $prefix/share/xbmc/web
#unzip -oq web/Project_Mayhem_III_webserver_v1.0.zip -d $prefix/share/xbmc/web
#
#
#echo "Copying XBMC binary to $prefix/share/xbmc/xbmc.bin"
#cp -f xbmc.bin $prefix/share/xbmc/xbmc.bin
#mkdir -p $prefix/bin
#cp tools/Linux/xbmc.sh $prefix/bin/xbmc
#cp tools/Linux/xbmc-standalone.sh $prefix/bin/xbmc-standalone
#cp tools/Linux/FEH.py $prefix/share/xbmc/FEH.py
#mkdir -p $prefix/share/xsessions
#cp tools/Linux/xbmc-xsession.desktop /usr/share/xsessions/XBMC.desktop
#chmod 755 $prefix/bin/xbmc
#chmod 755 $prefix/bin/xbmc-standalone
#echo "Copying support and legal files..."
#cp README.linux README.armel LICENSE.GPL *.txt xbmc-xrandr $prefix/share/xbmc/
#echo "Done!"


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
