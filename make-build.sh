#!/bin/sh
#
#linux shell script to create a ready-to-upload xbmc build
#from a cvs source tree containing compiled binaries
#requires rar, xbepatch, unix2dos in path

source="/xbmc/xbmc-src/XBMC"
base="/xbmc/release"
dest="$base/xbmc-`date +%Y-%m-%d`"
mkdir $dest

cd $source
cp Release/default.xbe $dest
cp *xml $dest
cp *txt $base
cp -r skin $dest

#cp /xbmc/home.xml $dest/skin/MediaCenter
# for replacing the default skin's home.xml with a customised version

#cp -r /xbmc/HiFi $dest/skin
#cp -r /xbmc/FlatStyle $dest/skin
# optional skins

cp -r language $dest
mkdir $dest/media
cp -r xbmc/keyboard/Media/* $dest/media
cp -r visualisations $dest
cp -r mplayer $dest
#mkdir $dest/mplayer/codecs
#cp /xbmc/win_dlls/*dll $dest/mplayer/codecs
# for adding codec DLLs (see readme.txt in that directory in cvs)

mkdir $dest/web
cd $dest/web
rar x $source/web/xbmc.rar >/dev/null

cd $base
unix2dos *txt
# XBMC can read unix format textfiles fine, this is just so the
# files display ok in crappy windows editors like notepad.exe

cd $dest
unix2dos *xml
rm -rf `find . -name "CVS*" -type d`
find skin -iname \*.png -exec rm -f "{}" \;
# xbmc only needs the XPR format skin files

xbepatch default.xbe retail.xbe
mv retail.xbe default.xbe
