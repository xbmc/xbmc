#!/bin/bash

_IFS=$IFS
_PROMPT_COMMAND=$PROMPT_COMMAND
echo -ne "]0;noT3CH"

BUILDDIR="./BUILD"
SOURCEDIR=$(expr  "$0" : '\(.*\)build\.sh')
(( BUILDEXISTS=0 ))
if [[ $SOURCEDIR == "" ]] 
then
  SOURCEDIR=.
fi

error() {
  if [[ $? != "0" ]]
  then
    echo " Errors have occured! Aborting."
    exit
  fi
}

clean() {
  if [ -d $1 ]
  then
    printf "\r Cleaning %-60.60s" $1
    rm -rf $I/src $I/.* $I/*.DLL $I/*.dll &> /dev/null
    for I in $(ls -d $1/* 2> /dev/null)
    do
      clean "${I}"
    done
  fi
}

if [[ -e $BUILDDIR ]]
then
  (( BUILDEXISTS=1 ))
  echo " Backing up UserData and scripts directories"
  echo "  If something goes wrong check ${SOURCEDIR}/.backup"
  mkdir -p $SOURCEDIR/.backup
  cp -r ${BUILDDIR}/UserData ${BUILDDIR}/scripts ${SOURCEDIR}/.backup
  if [[ $? != "0" ]]
  then
    echo " Backup failed! Please manually backup UserData"
    echo "  and scripts dirs then delete ${BUILDDIR}"
    exit
  fi
  echo " Removing old ${BUILDDIR}"
  rm -rf $BUILDDIR  &> /dev/null
fi

echo " Creating ${BUILDDIR}"
mkdir $BUILDDIR &> /dev/null

for I in credits language media screensavers scripts skin sounds system userdata visualisations web XboxMediaCenter
do
  printf "\r Copying %-16.16s" $I 
  if [[ $I == "skin" ]]
  then
    mkdir -p "${BUILDDIR}/skin/Project Mayhem III"
    for J in $(ls "${SOURCEDIR}/skin/Project Mayhem III")
    do
      if [[ $J == "media" ]]
      then
        mkdir "${BUILDDIR}/skin/Project Mayhem III/media"
        cp "${SOURCEDIR}/skin/Project Mayhem III/media/Textures.xpr" "${BUILDDIR}/skin/Project Mayhem III/media"
      else
        cp -r "${SOURCEDIR}/skin/Project Mayhem III/${J}" "${BUILDDIR}/skin/Project Mayhem III"
      fi
    done
  elif [[ $I == "userdata" ]]
  then
    if (( $BUILDEXISTS ))
    then
      cp -r ${SOURCEDIR}/.backup/UserData $BUILDDIR
    else
      cp -r ${SOURCEDIR}/${I} $BUILDDIR
    fi
  elif [[ $I == "scripts" ]] 
  then
    if (( $BUILDEXISTS ))
    then
      cp -r ${SOURCEDIR}/.backup/scripts $BUILDDIR
    else
      cp -r ${SOURCEDIR}/${I} $BUILDDIR
    fi
  else
    cp -r ${SOURCEDIR}/${I} $BUILDDIR
  fi
  error
done

printf "\r Copying %-16.16s\n" "complete!" 

echo " Fixing some case-sensitivity issues"
if (( ! $BUILDEXISTS ))
then
  echo "  Renaming userdata to UserData"
  mv ${BUILDDIR}/userdata ${BUILDDIR}/UserData
fi

echo "  Renaming fonts to Fonts"
mv "${BUILDDIR}/skin/Project Mayhem III/fonts" "${BUILDDIR}/skin/Project Mayhem III/Fonts"

echo "  Renaming arial.ttf to Arial.ttf"
mv ${BUILDDIR}/media/Fonts/arial.ttf ${BUILDDIR}/media/Fonts/Arial.ttf

echo "  Renaming Splash.png to splash.png"
mv ${BUILDDIR}/media/Splash.png ${BUILDDIR}/media/splash.png

IFS=$'\t\n'
clean "$BUILDDIR"
printf "\r Cleaning %-60.60s\n" "complete!"
if (( $BUILDEXISTS ))
then
  echo " Removing backups"
  rm -rf ${SOURCEDIR}/.backup
fi
echo " All done!"
IFS=$_IFS
$PROMPT_COMMAND

# A quicky for the ever impatient pike. More functionality quite soon
# Complain to AlTheKiller (ATKLap) in #xbmc-linux on FreeNode

