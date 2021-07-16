[[ -f $(dirname $0)/buildhelpers.sh ]] &&
    source $(dirname $0)/buildhelpers.sh

Win32BuildSetup=/xbmc/project/Win32BuildSetup
ERRORFILE=$Win32BuildSetup/errormingw
TOUCH=/bin/touch
RM=/bin/rm
NOPROMPT=0
MAKECLEAN=""
MAKEFLAGS=""
TRIPLET=x64

while true; do
  case $1 in
    --prompt=* ) PROMPTLEVEL="${1#*=}"; shift ;;
    --mode=* ) BUILDMODE="${1#*=}"; shift ;;
    -- ) shift; break ;;
    -* ) shift ;;
    * ) break ;;
  esac
done

export _WIN32_WINNT=0x0600
export NTDDI_VERSION=0x06000000
export TRIPLET

throwerror() {
  $TOUCH $ERRORFILE
  echo failed to compile $1
  if [ $NOPROMPT == 0 ]; then
	read
  fi
}

checkfiles() {
  for i in $@; do
    if [ ! -f "$PREFIX/$i" ]; then
      throwerror "$PREFIX/$i"
      exit 1
    fi
  done
}

buildProcess() {
export PREFIX=/xbmc/project/BuildDependencies/mingwlibs/$TRIPLET
if [ "$(pathChanged $PREFIX /xbmc/tools/buildsteps/windows /xbmc/tools/depends/target/ffmpeg/FFMPEG-VERSION)" == "0" ]; then
  return
fi

if [ -d "$PREFIX" ]; then
  rm -rdf $PREFIX/*
fi

cd /xbmc/tools/buildsteps/windows

# compile our mingw dlls
echo "-------------------------------------------------------------------------------"
echo " compiling mingw libs $TRIPLET"
echo
echo " NOPROMPT  = $NOPROMPT"
echo " MAKECLEAN = $MAKECLEAN"
echo " WORKSPACE = $WORKSPACE"
echo
echo "-------------------------------------------------------------------------------"

echo -ne "\033]0;building FFmpeg $TRIPLET\007"
echo "-------------------------------------------------"
echo " building FFmpeg $TRIPLET"
echo "-------------------------------------------------"
./buildffmpeg.sh $MAKECLEAN
checkfiles lib/avcodec.lib lib/avformat.lib lib/avutil.lib lib/postproc.lib lib/swscale.lib lib/avfilter.lib lib/swresample.lib
echo "-------------------------------------------------"
echo " building of FFmpeg $TRIPLET done..."
echo "-------------------------------------------------"
echo "-------------------------------------------------------------------------------"
echo " compile mingw libs $TRIPLET done..."
echo "-------------------------------------------------------------------------------"

tagSuccessFulBuild $PREFIX /xbmc/tools/buildsteps/windows /xbmc/tools/depends/target/ffmpeg/FFMPEG-VERSION
}

run_builds() {
    local profile_path=""
    profile_path=/local64/etc/profile.local

    if [ ! -z $profile_path ]; then
        if [[ ! -f "$profile_path" ]]; then
          echo "-------------------------------------------------------------------------------"
          echo " $TRIPLET build environment not configured, please run download-msys2.bat"
          echo "-------------------------------------------------------------------------------"
        else
          source $profile_path
          buildProcess
          echo "-------------------------------------------------------------------------------"
          echo " compile all libs $TRIPLET done..."
          echo "-------------------------------------------------------------------------------"
        fi
    fi
}

# cleanup
if [ -f $ERRORFILE ]; then
  $RM $ERRORFILE
fi

# check for noprompt
if [ "$PROMPTLEVEL" == "noprompt" ]; then
  NOPROMPT=1
fi

if [ "$BUILDMODE" == "clean" ]; then
  MAKECLEAN="clean"
else
  MAKECLEAN="noclean"
fi

run_builds

echo -e "\033]0;compiling done...\007"
echo

# wait for key press
if [ $NOPROMPT == 0 ]; then
  echo press a key to close the window
  read
fi
