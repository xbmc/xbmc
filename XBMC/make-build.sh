#!/bin/sh
#
#linux shell script to create a ready-to-install xbmc build
#from a cvs source tree containing compiled binaries

source="/xbmc/xbmc-src/XBMC"
base="/xbmc/build"
mkdir $base >/dev/null 2>&1
dest="$base/xbmc-`date +%Y-%m-%d`"
mkdir $dest

cd $source
cp Release/default.xbe $dest
cp *xml $dest
cp *txt $base
cp -r skin $dest
cp -r language $dest
cp -r weather $dest
mkdir $dest/credits
cp credits/* $dest/credits >/dev/null 2>&1
mkdir $dest/media
cp -r xbmc/keyboard/Media/* $dest/media
cp -r visualisations $dest
cp -r mplayer $dest

# win32 DLLs for wmv8/9, realmedia support (see XBMC/mplayer/codecs/readme.txt)
cp /xbmc/win_dlls/*dll $dest/mplayer/codecs

# images and other files needed for the webserver built into xbmc
mkdir $dest/web
cd $dest/web
rar x -inul /xbmc/xbmc-src/XBMC/web/xbmc.rar

# this is to make the txt and xml files display ok in crappy windows editors like
# notepad, xbmc itself reads unix or dos format without problems
cd $base
unix2dos *txt *nfo >/dev/null 2>&1
cd $dest
unix2dos *xml

find . -iname CVS\* -type d -exec rm -rf "{}" >/dev/null 2>&1 \;
find . -iname thumbs.db -or -name \*.h -type f -exec rm -f "{}" >/dev/null 2>&1 \;

# xbmc only loads textures.xpr, .pngs are not needed (except for weather)
find skin -iname \*.png -type f -exec rm -f "{}" \;

# to make the xbe work with very old modchips
xbepatch default.xbe retail.xbe
mv retail.xbe default.xbe
