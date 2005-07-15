#!/bin/sh
#
# $Header$
#unix shell script to create a ready-to-install xbmc build
#from a cvs source tree containing compiled binaries

source="/xbox/XBMC"
rbase="/xbox/xr"
name="xbmc-`date +%Y-%m-%d`"
dest="$rbase/$name"
mkdir -vp $dest

echo "---> making a XBMC release ..."
echo "compiled directory : [$source]"
echo "output directory   : [$dest]"

echo "---> copying system components ..."
cd $source
cp -v Release/default.xbe $dest

# to make the xbe work with very old modchips
# xbepatch default.xbe retail.xbe
# mv retail.xbe default.xbe

cp -v *.xml *.txt $dest
cp -rv skin $dest
cp -rv credits $dest
cp -rv language $dest
cp -rv screensavers $dest
cp -rv visualisations $dest
cp -rv system $dest
cp -rv media $dest
cp -rv sounds $dest
cp -rv python $dest
mkdir -vp $dest/web/
rar x web/web.rar $dest/web/

if [ ! -f $dest/system/players/paplayer/in_mp3.dll ]; then
  echo "missing in_mp3.dll" 
  echo "see system/players/paplayer/Place in_mp3.dll here.txt"
else
  rm -fv $dest/system/players/paplayer/Place\ in_mp3.dll\ here.txt
fi

# win32 DLLs for wmv8/9, realmedia and quicktime support 
# (see XBMC/system/players/mplayer/codecs/readme.txt)
cp -v /xbox/win_dlls/*dll $dest/system/players/mplayer/codecs/
cp -v /xbox/win_dlls/QuickTime* $dest/system/players/mplayer/codecs/

echo "---> making release leaner ..."
# make pm3 leaner
cd $dest/skin/Project\ Mayhem\ III/
rm -rfv 1080i 720p NTSC NTSC16x9 PAL PAL16x9
rm -rfv media/*.png media/*.jpg media/*.gif

# make credit leaner
rm -rfv $dest/credits/src
rm -v $dest/media/dsstdfx.bin

cd $dest
# make leaner
find . \( -name CVS -a -type d \) -exec rm -rf {} \; 
find . \( \( -name .cvsignore -o -name Thumbs.db -o -name .DS_Store \) \
-a -type f \) -exec rm -fv "{}" \;

# remove anything else e.g. extra languages etc


# make bundle
cd $rbase
rar a -r -m5 $name.rar $name
ls -l $name.rar
#tar cvfz $name.tar.gz $name
#ls -l $name.tar.gz

echo "---> XBMC release is ready!"

