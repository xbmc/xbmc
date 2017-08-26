[[ -f $(dirname $0)/buildhelpers.sh ]] &&
    source $(dirname $0)/buildhelpers.sh

Win32BuildSetup=/xbmc/project/Win32BuildSetup
ERRORFILE=$Win32BuildSetup/errormingw
TOUCH=/bin/touch
RM=/bin/rm
NOPROMPT=0
MAKECLEAN=""
MAKEFLAGS=""
TOOLS="mingw"
TRIPLET=""

while true; do
  case $1 in
    --tools=* ) TOOLS="${1#*=}"; shift ;;
    --build32=* ) build32="${1#*=}"; shift ;;
    --build64=* ) build64="${1#*=}"; shift ;;
    --buildArm=* ) buildArm="${1#*=}"; shift ;;
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
  export _WIN32_WINNT=0x0600
  export NTDDI_VERSION=0x06000000
elif [[ $win10 = "yes" ]]; then
  TRIPLET=$TRIPLET-uwp
fi

export TRIPLET ARCH TOOLS

throwerror() {
  $TOUCH $ERRORFILE
  echo failed to compile $1
  if [ $NOPROMPT == 0 ]; then
	read
  fi
}

checkfiles() {
  for i in $@; do
    if [ ! -f "$PREFIX/bin/$i" ]; then
      throwerror "$PREFIX/bin/$i"
      exit 1
    fi
  done
}

buildProcess() {
export PREFIX=/xbmc/project/BuildDependencies/mingwlibs/$TRIPLET
if [ "$(pathChanged $PREFIX /xbmc/tools/buildsteps/windows /xbmc/tools/depends/target/*/*-VERSION)" == "0" ]; then
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
echo " TOOLCHAIN = $TOOLS"
echo
echo "-------------------------------------------------------------------------------"

echo -ne "\033]0;building FFmpeg $TRIPLET\007"
echo "-------------------------------------------------"
echo " building FFmpeg $TRIPLET"
echo "-------------------------------------------------"
./buildffmpeg.sh $MAKECLEAN
checkfiles avcodec-57.dll avformat-57.dll avutil-55.dll postproc-54.dll swscale-4.dll avfilter-6.dll swresample-2.dll
echo "-------------------------------------------------"
echo " building of FFmpeg $TRIPLET done..."
echo "-------------------------------------------------"
if [[ $win10 != "yes" ]]; then # currently disabled for uwp
echo -ne "\033]0;building libdvd $TRIPLET\007"
echo "-------------------------------------------------"
echo " building libdvd $TRIPLET"
echo "-------------------------------------------------"
./buildlibdvd.sh $MAKECLEAN
checkfiles libdvdcss-2.dll libdvdnav.dll
echo "-------------------------------------------------"
echo " building of libdvd $TRIPLET done..."
echo "-------------------------------------------------"
fi
echo "-------------------------------------------------------------------------------"
echo " compile mingw libs $TRIPLET done..."
echo "-------------------------------------------------------------------------------"

tagSuccessFulBuild $PREFIX /xbmc/tools/buildsteps/windows /xbmc/tools/depends/target/*/*-VERSION
}

run_builds() {
    local profile_path=""
    if [[ $build32 = "yes" ]]; then
      profile_path=/local32/etc/profile.local
    elif [[ $build64 = "yes" ]]; then
      profile_path=/local64/etc/profile.local
    elif [[ $buildArm = "yes" ]]; then
      profile_path=/local32/etc/profile.local
    fi

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
