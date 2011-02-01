
ERRORFILE=/xbmc/project/Win32BuildSetup/errormingw
TOUCH=/bin/touch
RM=/bin/rm
NOPROMPT=0

function throwerror ()
{
  $TOUCH $ERRORFILE
  echo failed to compile $1
  if [ $NOPROMPT == 0 ]; then
	read
  fi
}

# cleanup
if [ -f $ERRORFILE ]; then
  $RM $ERRORFILE
fi

# compile our mingw dlls
echo "##### building ffmpeg dlls #####"
#sh /xbmc/lib/ffmpeg/build_xbmc_win32.sh
# check if dlls are build
for i in avcodec-52.dll avformat-52.dll avutil-50.dll postproc-51.dll swscale-0.6.1.dll; do
FILE=/xbmc/system/players/dvdplayer/$i
if [ ! -f $FILE ]; then
  throwerror "$FILE"
  exit 1
fi
done
echo "##### building of ffmpeg dlls done #####"

echo "##### building libdvd dlls #####"
sh /xbmc/lib/libdvd/build-xbmc-win32.sh

for i in libdvdcss-2.dll libdvdnav.dll; do
  FILE=/xbmc/system/players/dvdplayer/$i
  if [ ! -f $FILE ]; then
    throwerror "$FILE"
    exit 1
fi
done
echo "##### building of libdvd dlls done #####"

# wait for key press
if [ $NOPROMPT == 0 ]; then
  echo press a key to close the window
  read
fi


