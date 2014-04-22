
ERRORFILE=/xbmc/project/Win32BuildSetup/errormingw
NOPFILE=/xbmc/project/Win32BuildSetup/noprompt
MAKECLEANFILE=/xbmc/project/Win32BuildSetup/makeclean
BGPROCESSFILE=/xbmc/project/Win32BuildSetup/bgprocess
TOUCH=/bin/touch
RM=/bin/rm
NOPROMPT=0
MAKECLEAN=""
MAKEFLAGS=""

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

function runBackgroundProcess ()
{
  #start the process backgrounded
  $TOUCH $BGPROCESSFILE
  echo "backgrounding: sh $1 $BGPROCESSFILE & (workdir: $(PWD))"
  sh $1 $BGPROCESSFILE &
  echo "waiting on bgprocess..."
  while [ -f $BGPROCESSFILE ]; do
    echo -n "."
    sleep 5
  done
  echo "done"
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

if [ -f $MAKECLEANFILE ]; then
  $RM $MAKECLEANFILE
  MAKECLEAN="clean"
else
  MAKECLEAN="noclean"
fi

if [ $NUMBER_OF_PROCESSORS > 1 ]; then
  MAKEFLAGS=-j$NUMBER_OF_PROCESSORS
fi

# compile our mingw dlls
echo "################################"
echo "## compiling mingw libs"
echo "## NOPROMPT  = $NOPROMPT"
echo "## MAKECLEAN = $MAKECLEAN"
echo "################################"

echo "##### building ffmpeg dlls #####"
cd /xbmc/lib/ffmpeg/
runBackgroundProcess "./build_xbmc_win32.sh $MAKECLEAN"
setfilepath /xbmc/system/players/dvdplayer
checkfiles avcodec-53.dll avformat-53.dll avutil-51.dll postproc-52.dll swscale-2.dll avfilter-2.dll swresample-0.dll
echo "##### building of ffmpeg dlls done #####"

echo "##### building libdvd dlls #####"
cd /xbmc/lib/libdvd/
runBackgroundProcess "./build-xbmc-win32.sh $MAKECLEAN"
setfilepath /xbmc/system/players/dvdplayer
checkfiles libdvdcss-2.dll libdvdnav.dll
echo "##### building of libdvd dlls done #####"

echo "##### building libmpeg2 dlls #####"
cd /xbmc/lib/libmpeg2/
runBackgroundProcess "./make-xbmc-lib-win32.sh $MAKECLEAN"
setfilepath /xbmc/system/players/dvdplayer
checkfiles libmpeg2-0.dll
echo "##### building of libmpeg2 dlls done #####"

echo "##### building timidity dlls #####"
cd /xbmc/lib/timidity/
if  [ "$MAKECLEAN" == "clean" ]; then
  make -f Makefile.win32 clean
fi
make -f Makefile.win32 $MAKEFLAGS
setfilepath /xbmc/system/players/paplayer
checkfiles timidity.dll
echo "##### building of timidity dlls done #####"

echo "##### building asap dlls #####"
cd /xbmc/lib/asap/win32
runBackgroundProcess "./build_xbmc_win32.sh $MAKECLEAN"
setfilepath /xbmc/system/players/paplayer
checkfiles xbmc_asap.dll
echo "##### building of asap dlls done #####"

# wait for key press
if [ $NOPROMPT == 0 ]; then
  echo press a key to close the window
  read
fi
