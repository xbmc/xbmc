[[ -f $(dirname $0)/buildhelpers.sh ]] &&
    source $(dirname $0)/buildhelpers.sh

Win32BuildSetup=/xbmc/project/Win32BuildSetup
ERRORFILE=$Win32BuildSetup/errormingw
TOUCH=/bin/touch
RM=/bin/rm
NOPROMPT=0
MAKECLEAN=""
MAKEFLAGS=""
TRIPLET=""

while true; do
  case $1 in
    --build32=* ) build32="${1#*=}"; shift ;;
    --build64=* ) build64="${1#*=}"; shift ;;
    --buildArm=* ) buildArm="${1#*=}"; shift ;;
    --buildArm64=* ) buildArm64="${1#*=}"; shift ;;
    --prompt=* ) PROMPTLEVEL="${1#*=}"; shift ;;
    --mode=* ) BUILDMODE="${1#*=}"; shift ;;
    --win10=* ) win10="${1#*=}"; shift ;;
    -- ) shift; break ;;
    -* ) shift ;;
    * ) break ;;
  esac
done

if [[ $build32 = "yes" ]]; then
  TRIPLET=win32
  ARCH=x86
elif [[ $build64 = "yes" ]]; then
  TRIPLET=x64
  ARCH=x86_64
elif [[ $buildArm = "yes" ]]; then
  TRIPLET=arm
  ARCH=arm
elif [[ $buildArm64 = "yes" ]]; then
  TRIPLET=arm64
  ARCH=arm64
else
  echo "-------------------------------------------------------------------------------"
  echo " none of build types (build32, build64 or buildArm) was specified "
  echo "-------------------------------------------------------------------------------"
  # wait for key press
  if [ "$PROMPTLEVEL" != "noprompt" ]; then
    echo press a key to close the window
    read
  fi
  exit
fi

if [[ $win10 = "no" ]]; then
  export _WIN32_WINNT=_WIN32_WINNT_WINBLUE
  export NTDDI_VERSION=NTDDI_WINBLUE
elif [[ $win10 = "yes" ]]; then
  TRIPLET=win10-$TRIPLET
fi

export TRIPLET ARCH

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
if [ "$(pathChanged $PREFIX /xbmc/tools/depends/target/ffmpeg /xbmc/tools/buildsteps/windows /xbmc/tools/depends/target/dav1d/DAV1D-VERSION /xbmc/tools/depends/target/ffmpeg/FFMPEG-VERSION)" == "0" ]; then
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

# Build dav1d explicitly first; FFmpeg's configure checks for libdav1d via
# `require libdav1d ... -llibdav1d` and bails out with "ERROR: libdav1d not
# found" if the import library is not present in $PREFIX/lib.
echo -ne "\033]0;building dav1d $TRIPLET\007"
echo "-------------------------------------------------"
echo " building dav1d $TRIPLET"
echo "-------------------------------------------------"
./builddav1d.sh $MAKECLEAN
checkfiles lib/libdav1d.lib
echo "-------------------------------------------------"
echo " building of dav1d $TRIPLET done..."
echo "-------------------------------------------------"

echo -ne "\033]0;building FFmpeg $TRIPLET\007"
echo "-------------------------------------------------"
echo " building FFmpeg $TRIPLET"
echo "-------------------------------------------------"
FFMPEG_LOG="$Win32BuildSetup/ffmpeg-build.log"
./buildffmpeg.sh $MAKECLEAN 2>&1 | tee "$FFMPEG_LOG"
FFMPEG_RC=${PIPESTATUS[0]}
echo "FFmpeg exit code: $FFMPEG_RC"
echo "Full FFmpeg log saved to: $FFMPEG_LOG"
if [[ $FFMPEG_RC -ne 0 ]]; then
  echo "=== last 60 lines of FFmpeg log ==="
  tail -n 60 "$FFMPEG_LOG"
  echo "==================================="
  exit $FFMPEG_RC
fi
checkfiles lib/avcodec.lib lib/avformat.lib lib/avutil.lib lib/postproc.lib lib/swscale.lib lib/avfilter.lib lib/swresample.lib
echo "-------------------------------------------------"
echo " building of FFmpeg $TRIPLET done..."
echo "-------------------------------------------------"
echo "-------------------------------------------------------------------------------"
echo " compile mingw libs $TRIPLET done..."
echo "-------------------------------------------------------------------------------"

tagSuccessFulBuild $PREFIX /xbmc/tools/depends/target/ffmpeg /xbmc/tools/buildsteps/windows /xbmc/tools/depends/target/dav1d/DAV1D-VERSION /xbmc/tools/depends/target/ffmpeg/FFMPEG-VERSION
}

run_builds() {
    local profile_path=""
    if [[ $build32 = "yes" ]]; then
      profile_path=/local32/etc/profile.local
    elif [[ $build64 = "yes" ]]; then
      profile_path=/local64/etc/profile.local
    elif [[ $buildArm = "yes" ]]; then
      profile_path=/local32/etc/profile.local
    elif [[ $buildArm64 = "yes" ]]; then
      profile_path=/localarm64/etc/profile.local
    fi

    if [ ! -z $profile_path ]; then
        if [[ ! -f "$profile_path" ]]; then
          echo "-------------------------------------------------------------------------------"
          echo " $TRIPLET build environment not configured, please run download-msys2.bat"
          echo "-------------------------------------------------------------------------------"
          exit 1
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
