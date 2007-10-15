#!/bin/bash

error() {
  if [[ $? != 0 ]]
  then
    echo
    echo " FAILED! Exiting."
    exit
  fi
}

usage() {
  echo " build.sh by default checks that your source is up-to-date, updates it"
  echo "  if not, compile, and create a working build of XBMC in ./BUILD."
  echo "  UserData and scripts dirs as well as 3rd party skins will be backed"
  echo "  up if existing."
  echo " Usage: build.sh [OPTIONS]"
  echo "  OPTIONS:"
  echo "   --help [-h]                      : Display this text."
  echo "   DEST=<build-dest>                : Path to install XBMC to"
  echo "   NOUPDATE                         : Don't update source."
  echo "   NOCOMPILE                        : Don't compile."
  echo "   NOCLEAN                          : Don't run \"make clean\" first."
  echo "   NOCOPY                           : Don't create XBMC file structure."
  echo "   NODEBUG                          : Don't create debugging info, strip binary."
  echo "   CONFIRM                          : Don't ask about anything"
#  echo "   SHOWMAKE                         : Don't suppress make output"
## SHOWMAKE requires changes to Makefile.in
  echo "   NOCONFIG                         : Don't automatically run configure"
  echo "   CONFIGOPT=<config-option>        : Option to pass to configure."
  echo "                                      One option per CONFIGOPT=,"
  echo "                                      can pass more than one"
  echo "   WEB=<path/to/web_int.rar>        : Web interface to use."
  echo "                                      Default = PM3"
  echo " These options can be defaulted in ~/.xbmc-build-settings."
  echo " Just make a white space separated list on the first line."
  exit
}

view_log() {
  PROMPT=""
  while [[ $PROMPT != "y" && $PROMPT != "n" ]]
  do
    printf "\r View compile log? (y/n) :  \b"
    read -n 1 PROMPT
  done
  echo
  if [[ $PROMPT = "y" ]]
  then
    less "${SOURCEDIR}/compile.log"
  fi
  exit
}

parse_args() {
  for I in "$@"
  do
    OPT=${I%=*}
    PAR=${I#*=}
    case $OPT in
      --help|-h)
        usage
        ;;
      DEST)
          BUILDDIR=$PAR
        ;;
      NOUPDATE)
        (( UPDATE=0 ))
        ;;
      NOCOMPILE)
        (( COMPILE=0 ))
        ;;
      NOCLEAN)
        (( CLEAN=0 ))
        ;;
      NOCOPY)
        (( COPY=0 ))
        ;;
      CONFIRM)
        (( CONFIRM=1 ))
        ;;
      SHOWMAKE)
        (( SHOW_MAKE=1 ))
        ;;
      CONFIGOPT)
        (( CONFIGURE=1 ))
        CONFIGOPTS="$CONFIGOPTS $PAR"
        ;;
      NODEBUG)
        (( DEBUG=0 ))
	      CONFIGOPTS="$CONFIGOPTS --disable-debug"
        ;;
      WEB)
        if [[ -e $PAR ]]
        then
          WEB=$PAR
        else
          echo "  $PAR doesn't exist!"
          exit
        fi
        ;;
      NOCONFIG)
        CONFIGURE=0
        ;;
      *)
        echo " Invalid option $OPT"
        usage
        ;;
    esac
  done
}

update() {
  echo " Local source revision : $LOCAL_REVISION"
  echo -n " Repository revision : Checking..."
  HEAD_REVISION=$(expr "$(svn info "$SOURCEDIR" -r HEAD 2>&1 | grep "Revision")" : '.*: \(.*\)')
  printf "\r Repository revision : %-11.11s\n" $HEAD_REVISION
  
  if [[ $LOCAL_REVISION = "" || $HEAD_REVISION = "" ]]
  then
    echo " Couldn't determine if source is upto date!"
    echo " This is probably due to network errors, skipping update."
    PROMPT=""
    while [[ $PROMPT != "y" && $PROMPT != "n" ]]
    do
      printf "\r Continue? (y/n) :  \b"
      read -n 1 PROMPT
    done
    echo
    if [[ $PROMPT == "n" ]]
    then
      exit
    fi
  else
    if (( $LOCAL_REVISION < $HEAD_REVISION ))
    then
      if (( CONFIRM ))
      then
        PROMPT="y"
      else
        PROMPT=""
      fi
      echo " Your source is outdated."
      while [[ $PROMPT != "y" && $PROMPT != "n" ]]
      do
        printf "\r Update? (y/n) :  \b"
        read -n 1 PROMPT
      done
      if ! (( CONFIRM ))
      then
        echo
      fi
      if [[ $PROMPT == "y" ]]
      then
        echo " Updating source code."
        svn up "$SOURCEDIR" 2>&1 | tee .build.sh.svn
        grep "^Updated to" .build.sh.svn &> /dev/null
        error
        grep -E "configure$|\.in$" .build.sh.svn &> /dev/null
        if ! (($?))
        then
          CONFIGURE=1
        fi
        LOCAL_REVISION="$HEADREVISION"
      fi
    else
      echo " Your source is up to date."
    fi
  fi
}

config() {
  if [[ $CONFIGOPTS = "" ]]
  then
    echo " Configuring build."
  else
    echo " Configuring build with $CONFIGOPTS."
  fi
  DIR=$PWD
  cd "$SOURCEDIR"
  ./configure $CONFIGOPTS
  cd "$DIR"
}

compile() {
  if (( CONFIGURE )) 
  then
    config
  fi
  if [[ -e "$SOURCEDIR/XboxMediaCenter" ]]
  then
    echo " A compiled executable already exists."
    if (( CONFIRM ))
    then
      PROMPT="y"
    else
      PROMPT=""
    fi
    while [[ $PROMPT != "y" && $PROMPT != "n" ]]
    do
      printf "\r Compile anyway? (y/n) :  \b"
      read -n 1 PROMPT
    done
    if ! (( CONFIRM ))
    then
      echo
    fi
    if [[ $PROMPT == "n" ]]
    then
      (( COMPILE=0 ))
      echo " Skipping compilation."
    fi
  fi
  if (( COMPILE ))
  then
    if ! [[ -e "$SOURCEDIR/Makefile" ]]
    then
      config
    fi
    if (( CLEAN ))
    then
      echo " Cleaning source directory."
      make -C "${SOURCEDIR}" clean &> /dev/null
    else
      echo " Skipping source directory cleaning."
    fi
    echo " Compiling source."
    CORES=$(grep "processor" /proc/cpuinfo | wc -l)
    echo "  Detected ${CORES} procs/cores, using -j${CORES}"
    if (( $SHOW_MAKE ))
    then
      make -j${CORES} -C "${SOURCEDIR}" 2>&1 | tee "${SOURCEDIR}/compile.log"
    else
      make -j${CORES} -C "${SOURCEDIR}" 2>&1 | tee "${SOURCEDIR}/compile.log" | grep -E "Linking|Building|Compiling"
    fi
   
    grep Error "${SOURCEDIR}/compile.log"
    if [[ $? == "0" ]]
    then
      echo
      echo " Errors have occurred!"
      view_log
    fi
  fi
}

copy() {
  BACKUPDIR="${BUILDDIR}.bak"
  if [[ -e "$BUILDDIR" ]]
  then
    #backup
    echo " Backing up old ${BUILDDIR} to ${BACKUPDIR}."
    if [[ -e "$BACKUPDIR" ]] 
    then
      echo "  Removing old $BACKUPDIR first."
      rm -rf "$BACKUPDIR" &> /dev/null
    fi
    mv "$BUILDDIR" "${BACKUPDIR}"  &> /dev/null
    if [[ $? != 0 ]]
    then
      echo " You don't have permission to move"
      echo "  the old $BUILDDIR. Please fix this"
      echo "  and rerun with options NOUPDATE"
      echo "  and NOCOMPILE"
      exit
    fi
  fi

  echo " Creating ${BUILDDIR}."
  mkdir "$BUILDDIR" &> /dev/null
  error

  for I in credits language media screensavers scripts skin sounds system userdata visualisations web XboxMediaCenter README.linux demo-asoundrc copying.txt Changelog.txt
  do
    printf "\r Copying %-16.16s" $I 
    if [[ "$I" == "skin" ]]
    then
      mkdir -p "${BUILDDIR}/skin/Project Mayhem III" &> /dev/null
      for J in $(ls "${SOURCEDIR}/skin/Project Mayhem III")
      do
        if [[ "$J" == "media" ]]
        then
          mkdir "${BUILDDIR}/skin/Project Mayhem III/media" &> /dev/null
          cp "${SOURCEDIR}/skin/Project Mayhem III/media/Textures.xpr" "${BUILDDIR}/skin/Project Mayhem III/media" &> /dev/null
        else
          cp -rf "${SOURCEDIR}/skin/Project Mayhem III/${J}" "${BUILDDIR}/skin/Project Mayhem III" &> /dev/null
        fi
      done
    elif [[ "$I" == "userdata" ]]
    then
      if [[ -e "$BACKUPDIR/UserData" ]]
      then
        cp -rf "$BACKUPDIR/UserData" "$BUILDDIR" &> /dev/null
      else
        cp -rf "${SOURCEDIR}/${I}" "$BUILDDIR" &> /dev/null
      fi
    elif [[ "$I" == "web" ]]
    then
      RAR="$(which unrar)"
      if [[ $RAR == "" ]]
      then
        RAR="$(which rar)"
      fi
      if ! [[ $RAR == "" ]]
      then
        mkdir -p "$BUILDDIR/web" &> /dev/null
        "$RAR" x -y -inul "$WEB" "$BUILDDIR/web/"
      fi
    elif [[ "$I" == "XboxMediaCenter" ]]
    then
      if [[ -e "${SOURCEDIR}/$I" ]]
      then
        mv "${SOURCEDIR}/${I}" "$BUILDDIR" &> /dev/null
      elif [[ -e "${BACKUPDIR}/${I}" ]]
      then
        echo
        echo " Couldn't find new binary, using old one from backup!"
        cp -f "${BACKUPDIR}/${I}" "${BUILDDIR}/${I}" &> /dev/null
      else 
        ls "..." &> /dev/null  # force $? to be non-zero
      fi
    else
      cp -rf "${SOURCEDIR}/${I}" "$BUILDDIR" &> /dev/null
    fi
    error
  done
  
  if ! (( DEBUG ))
  then
    echo " Stripping binary."
    strip "$BUILDDIR/XboxMediaCenter"
  fi

  printf "\r Copying %-16.16s\n" "complete!" 
  if [[ $RAR == "" ]]
  then
    echo "  Couldn't find \"rar\" or \"unrar\" please install one to use web interface"
  fi
}

cleanup() {
  if [ -d $1 ]
  then
    printf "\r Cleaning %-60.60s" $1
    rm -rf $I/src $I/.svn $I/*.DLL $I/*.dll &> /dev/null
    for I in $1/* #$(ls -d $1/* 2> /dev/null)
    do
      cleanup "${I}"
    done
  fi
}

merge() {
  if [[ -d "$BACKUPDIR/$1" ]]
  then
    if [[ -e "$BUILDDIR/$1" ]]
    then
      for I in $(ls -d $BACKUPDIR/$1/* 2> /dev/null)
      do
        I=${I#"${BACKUPDIR}/"}
        merge "$I"
      done
    else
      echo "  Merged ${1#"${BACKUPDIR}/"}"
      cp -rf "$BACKUPDIR/$1" "$BUILDDIR/$1" &> /dev/null
    fi
  else
    if ! [[ -e "$BUILDDIR/$1" ]]
    then
      echo "  Merged ${1#"${BACKUPDIR}/"}"
      cp -f "$BACKUPDIR/$1" "$BUILDDIR/$1" &> /dev/null
    fi
  fi
}

fix() {
  echo " Fixing some case-sensitivity issues."
  if [[ -e "$BUILDDIR/userdata" ]]
  then
    echo "  Renaming userdata to UserData."
    mv "${BUILDDIR}/userdata" "${BUILDDIR}/UserData"
  fi

  echo "  Renaming arial.ttf to Arial.ttf."
  mv  "${BUILDDIR}/media/Fonts/arial.ttf" "${BUILDDIR}/media/Fonts/Arial.ttf"

  echo "  Renaming Splash.png to splash.png."
  mv "${BUILDDIR}/media/Splash.png" "${BUILDDIR}/media/splash.png"

  _IFS=$IFS
  IFS=$'\t\n'
  cleanup "$BUILDDIR"
  printf "\r Cleaning %-60.60s\n" "complete!"
  IFS=$_IFS
  if [[ -e "$BACKUPDIR" ]]
  then
    echo " Merging 3rd party files from backup."
    for I in $(ls $BACKUPDIR) #screensavers scripts skin sounds system visualisations
    do
      IFS=$'\t\n'
      merge "$I"
      IFS=$_IFS
    done
    echo " Merge complete!"
  fi
}

SOURCEDIR=${0%/*}
BACKUPDIR="$SOURCEDIR/.backup"
# Don't touch these. Make a ~/.xbmc-build-settings instead.
# See ./build.sh --help
BUILDDIR="./BUILD"
WEB=""
CONFIGOPTS=""
(( UPDATE=1 ))
(( COMPILE=1 ))
(( CLEAN=1 ))
(( COPY=1 ))
(( CONFIRM=0 ))
(( SHOW_MAKE=1 ))
(( CONFIGURE=1 ))
(( DEBUG=1 ))

if ! [[ -e "$SOURCEDIR/.firstrun" ]]
then
  touch "$SOURCEDIR/.firstrun"
  echo
  echo "*** FIRST RUN DETECTED. YOU'D BETTER READ THIS CAREFULLY! ***"
  echo
  usage
fi

echo -ne "]0;Building XBMC"

touch "/root/.test" &> /dev/null

if ! (( $? )) 
then
  PROMPT=""
  echo " There is really no reason to run this as root or with sudo."
  while [[ $PROMPT != "y" && $PROMPT != "n" ]]
  do
    printf "\r Run anyway? (y/n) :  \b"
    read -n 1 PROMPT
  done
  echo
  if [[ $PROMPT = "n" ]]
  then
    exit
  fi
fi

if [[ -e ~/.xbmc-build-settings ]]
then
  SETTINGS=$(head -n1 ~/.xbmc-build-settings)
  parse_args $SETTINGS
fi

parse_args "$@"

if [[ "${BUILDDIR:(-1)}" = "/" ]]
then
  BUILDDIR="${BUILDDIR%/}"
fi

LOCAL_REVISION=$(expr "$(svn info "$SOURCEDIR" 2>&1 | grep "Revision")" : '.*: \(.*\)')

if [[ -e "$BUILDDIR" ]]
then
  if ! [[ -dwr "$BUILDDIR" ]]
  then
    echo " $BUILDDIR exists, but you can't modify it or it is a file."
    exit
  fi
else
  mkdir -p "$BUILDDIR" &> /dev/null
  if [[ $? != 0 ]]
  then
    echo " You don't have permission to create $BUILDDIR"
    exit
  fi
  rm -rf "$BUILDDIR" &> /dev/null
fi

if [[ $WEB == "" ]]
then
  WEB="$SOURCEDIR/web/Project_Mayem_III_webserver_v1.0.rar"
fi

if (( UPDATE ))
then
  update
else
  echo " Skipping update."
fi

if (( COMPILE ))
then
  compile
else
  echo " Skipping compile."
fi

if (( UPDATE || COMPILE || COPY ))
then
  echo " Generating Changelog.txt"
  "$SOURCEDIR/tools/Changelog/Changelog.py" -r $LOCAL_REVISION -d "$SOURCEDIR"
fi

if (( COPY ))
then
  copy
  fix
else
  echo " Skipping XBMC file structure creation."
fi

echo " All done!"

if (( COMPILE && !CONFIRM))
then
  view_log
fi

