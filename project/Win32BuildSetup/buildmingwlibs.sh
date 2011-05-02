
ERRORFILE=/xbmc/project/Win32BuildSetup/errormingw
NOPFILE=/xbmc/project/Win32BuildSetup/noprompt
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

function setfilepath ()
{
  FILEPATH=$1
}

function checkfiles ()
{
  for i in $@; do
  FILE=$FILEPATH/$i
  if [ ! -f $FILE ]; then
    throwerror "$FILE"
    exit 1
  fi
  done
}

# cleanup
if [ -f $ERRORFILE ]; then
  $RM $ERRORFILE
fi

# check for noprompt
if [ -f $NOPFILE ]; then
  $RM $NOPFILE
  NOPROMPT=1
fi

# compile our mingw dlls
echo "##### building ffmpeg dlls #####"
cd /xbmc/lib/ffmpeg/
sh ./build_xbmc_win32.sh
setfilepath /xbmc/system/players/dvdplayer
checkfiles avcodec-52.dll avcore-0.dll avformat-52.dll avutil-50.dll postproc-51.dll swscale-0.6.1.dll
echo "##### building of ffmpeg dlls done #####"

echo "##### building libdvd dlls #####"
cd /xbmc/lib/libdvd/
sh ./build-xbmc-win32.sh
setfilepath /xbmc/system/players/dvdplayer
checkfiles libdvdcss-2.dll libdvdnav.dll
echo "##### building of libdvd dlls done #####"

echo "##### building libmpeg2 dlls #####"
cd /xbmc/lib/libmpeg2/
sh ./make-xbmc-lib-win32.sh
setfilepath /xbmc/system/players/dvdplayer
checkfiles libmpeg2-0.dll
echo "##### building of libmpeg2 dlls done #####"

echo "##### building timidity dlls #####"
cd /xbmc/lib/timidity/
make -f Makefile.win32 clean
make -f Makefile.win32
setfilepath /xbmc/system/players/paplayer
checkfiles timidity.dll
echo "##### building of timidity dlls done #####"

echo "##### building asap dlls #####"
cd /xbmc/lib/asap/win32
sh ./build_xbmc_win32.sh
setfilepath /xbmc/system/players/paplayer
checkfiles xbmc_asap.dll
echo "##### building of asap dlls done #####"

# wait for key press
if [ $NOPROMPT == 0 ]; then
  echo press a key to close the window
  read
fi
