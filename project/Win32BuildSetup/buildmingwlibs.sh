#
# buildmingwlibs.sh (v1.1) - updated to support ffmpeg bump to 1.2
#

WIN32SETUP=/xbmc/project/Win32BuildSetup
ERRORFILE=$WIN32SETUP/errormingw
PROMPTFILE=$WIN32SETUP/noprompt
MAKECLEANFILE=$WIN32SETUP/makeclean
MINGWLIBSOK=$WIN32SETUP/mingwlibsok

TOUCH=/bin/touch
RM=/bin/rm
PROMPT=0
MAKECLEAN=""
MAKEFLAGS=""

#
# Function declarations. (start)
#

function throwerror ()
{
  echo failed to compile $FILEPATH/$1 | tee -a $ERRORFILE

  if [ $PROMPT == 1 ]; then
	read
  fi
}

function checkfiles ()
{
  for FILE in $@
  do
     if [ ! -f $FILE ]; then
	rm -f $MINGWLIBSOK
	throwerror "$FILE"
	exit 1
     fi
  done
}
#
# Function declarations. (end)
#



#
# Main...
#

# cleanup
if [ -f $ERRORFILE ]; then
  $RM $ERRORFILE
fi

# check for prompt file
if [ -f $PROMPTFILE ]; then
  $RM $PROMPTFILE
  PROMPT=1
fi

if [ -f $MAKECLEANFILE ]; then
  $RM $MAKECLEANFILE
  MAKECLEAN="clean"
fi

if [ $NUMBER_OF_PROCESSORS > 1 ]; then
  MAKEFLAGS=-j$NUMBER_OF_PROCESSORS
fi

# compile our mingw dlls
echo "################################"
echo "## compiling mingw libs"
echo "## PROMPT    = $PROMPT"
echo "## MAKECLEAN = $MAKECLEAN"
echo "################################"

echo "##### building ffmpeg dlls #####"
cd /xbmc/lib/ffmpeg
if sh ./build_xbmc_win32.sh $MAKECLEAN
then
   echo "##### building of ffmpeg dlls done #####"
else
   throwerror "building the ffmpeg dlls failed!"
   exit 1
fi

cd /xbmc/system/players/dvdplayer

LIBAVCODEC=$(set -- avcodec-[0-9]*.dll; echo $1)
LIBAVFORMAT=$(set -- avformat-[0-9]*.dll; echo $1)
LIBAVUTIL=$(set --  avutil-[0-9]*.dll; echo $1)
LIBAVFILTER=$(set -- avfilter-[0-9]*.dll; echo $1)
LIBPOSTPROC=$(set -- postproc-[0-9]*.dll; echo $1)
LIBSWSCALE=$(set -- swscale-[0-9]*.dll; echo $1)
LIBSWRESAMPLE=$(set -- swresample-[0-9]*.dll; echo $1)

checkfiles $LIBAVCODEC $LIBAVFORMAT $LIBAVUTIL $LIBAVFILTER $LIBPOSTPROC $LIBSWSCALE $LIBSWRESAMPLE

echo "##### building libdvd dlls #####"
cd /xbmc/lib/libdvd/
sh ./build-xbmc-win32.sh $MAKECLEAN
cd /xbmc/system/players/dvdplayer
checkfiles libdvdcss-2.dll libdvdnav.dll
echo "##### building of libdvd dlls done #####"

echo "##### building libmpeg2 dlls #####"
cd /xbmc/lib/libmpeg2/
sh ./make-xbmc-lib-win32.sh $MAKECLEAN
cd /xbmc/system/players/dvdplayer
checkfiles libmpeg2-0.dll
echo "##### building of libmpeg2 dlls done #####"

echo "##### building timidity dlls #####"
cd /xbmc/lib/timidity/
if  [ "$MAKECLEAN" == "clean" ]; then
  make -f Makefile.win32 clean
fi
make -f Makefile.win32 $MAKEFLAGS
cd /xbmc/system/players/paplayer
checkfiles timidity.dll
echo "##### building of timidity dlls done #####"

echo "##### building asap dlls #####"
cd /xbmc/lib/asap/win32
sh ./build_xbmc_win32.sh $MAKECLEAN
cd /xbmc/system/players/paplayer
checkfiles xbmc_asap.dll
echo "##### building of asap dlls done #####"

touch $MINGWLIBSOK

# wait for key press
if [ $PROMPT == 1 ]; then
  echo press a key to close the window
  read
fi
