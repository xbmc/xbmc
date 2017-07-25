#!/bin/bash

MAKEFLAGS="$1"
BGPROCESSFILE="$2"
tools="$3"

cpuCount=1
if [[ $NUMBER_OF_PROCESSORS > 1 ]]; then
  if [[ $NUMBER_OF_PROCESSORS > 4 ]]; then
    cpuCount=$NUMBER_OF_PROCESSORS
  else
    cpuCount=`expr $NUMBER_OF_PROCESSORS + $NUMBER_OF_PROCESSORS / 2`
  fi
fi
if [[ ! $cpuCount =~ ^[0-9]+$ ]]; then
  cpuCount="$(($(nproc)/2))"
fi

if which tput >/dev/null 2>&1; then
    ncolors=$(tput colors)
    if test -n "$ncolors" && test "$ncolors" -ge 8; then
        bold_color=$(tput bold)
        blue_color=$(tput setaf 4)
        orange_color=$(tput setaf 3)
        green_color=$(tput setaf 2)
        red_color=$(tput setaf 1)
        reset_color=$(tput sgr0)
    fi
    ncols=72
fi

if [[ ! -d /build/src ]]; then
  mkdir /build/src
fi

do_wget() {
  local URL="$1"
  local archive="$2"

  if [[ -z $archive ]]; then
    wget --tries=5 --retry-connrefused --waitretry=2 --no-check-certificate -c -P /downloads/ $URL
  else
    wget --tries=5 --retry-connrefused --waitretry=2 --no-check-certificate -c $URL -O /downloads/$archive
  fi
}

do_makeinstall() {
  make -j"$cpuCount" "$@"
  make install
}

do_makelib() {
  do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$blue_color" "Compiling"
  do_makeinstall $1
  if [ $? == 0 ]; then
    do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$green_color" "Done"
  else
    do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$red_color" "Error"
  fi
}

do_print_status() {
  local pad=$(printf '%0.1s' "."{1..72})
  local padlen=$((${#pad}-${#1}-${#3}))
  printf '%s %*.*s%s%s%s\n' "${bold_color}$1${reset_color}" 0 "$padlen" "$pad" " [$2" "$3" "$reset_color]"
}

do_print_progress() {
  echo -e "\e]0;$* in $(get_first_subdir)\007"
  echo -e "${bold_color}$* in $(get_first_subdir)${reset_color}"
}

get_first_subdir() {
  local subdir="${PWD#*build/}"
  if [[ "$subdir" != "$PWD" ]]; then
    subdir="${subdir%%/*}"
    echo "$subdir"
  else
    echo "."
  fi
}

do_pkgConfig() {
  local pkg=${1%% *}
  local version=$2
  [[ -z "$version" ]] && version="${1##*= }"
  [[ "$version" = "$1" ]] && version="" || version=" $version"
  local prefix=$(pkg-config --variable=prefix --silence-errors "$1")
  [[ ! -z "$prefix" ]] && prefix="$(cygpath -u "$prefix")"
  if [[ "$prefix" = "$LOCALDESTDIR" || "$prefix" = "/trunk${LOCALDESTDIR}" ]]; then
    do_print_status "${pkg} ${version}" "$green_color" "Up-to-date"
    return 1
  else
    do_print_status "${pkg} ${version}" "$red_color" "Not installed"
  fi
}

do_autoreconf() {
  if [[ ! -f $LOCALSRCDIR/configure ]]; then 
    local CURDIR=$(pwd)
    cd $LOCALSRCDIR
    autoreconf -fiv
    cd $CURDIR
  fi
}

do_clean() {
  if [[ "$1" == "clean" ]]; then
    if [ -d "$LIBBUILDDIR" ]; then
      do_print_status "$LIBNAME ($TRIPLET)" "$red_color" "Removing"
      rm -rf "$LIBBUILDDIR"
    fi
  fi
}

do_download() {
  if [ ! -d "$LOCALSRCDIR" ]; then
    if [ ! -f /downloads/$ARCHIVE ]; then
      do_print_status "$LIBNAME-$VERSION" "$orange_color" "Downloading"
      do_wget $BASE_URL/$VERSION.tar.gz $ARCHIVE
    fi

    do_print_status "$LIBNAME-$VERSION" "$blue_color" "Extracting"
    mkdir $LOCALSRCDIR && cd $LOCALSRCDIR
    tar -xaf /downloads/$ARCHIVE --strip 1
  fi
  # applying patches
  local patches=(/xbmc/tools/buildsteps/windows/patches/*-$LIBNAME-*.patch)
  for patch in ${patches[@]}; do
    echo "Applying patch ${patch}"
    if [[ -f $patch ]]; then
      patch -d $LOCALSRCDIR -i $patch -N -r -
    fi
  done
}

do_loaddeps() {
  local file="$1"
  LIBNAME=$(grep "LIBNAME=" $file | sed 's/LIBNAME=//g;s/#.*$//g;/^$/d')
  BASE_URL=$(grep "BASE_URL=" $file | sed 's/BASE_URL=//g;s/#.*$//g;/^$/d')
  VERSION=$(grep "VERSION=" $file | sed 's/VERSION=//g;s/#.*$//g;/^$/d')
  GNUTLS_VER=$(grep "GNUTLS_VER=" $file | sed 's/GNUTLS_VER=//g;s/#.*$//g;/^$/d')
  GITREV=$(git ls-remote $BASE_URL $VERSION | awk '{print substr($1, 1, 10)}')
  if [[ -z "$GITREV" ]]; then
    ARCHIVE=$LIBNAME-$(echo "${VERSION}" | sed 's/\//-/g').tar.gz
  else
    ARCHIVE=$LIBNAME-$GITREV.tar.gz
  fi
  BASE_URL=$BASE_URL/archive
  local libsrcdir=$LIBNAME-$VERSION
  if [[ ! -z "$GITREV" ]]; then
    libsrcdir=$LIBNAME-$GITREV
  fi
  LOCALSRCDIR=$LOCALBUILDDIR/src/$libsrcdir
  LIBBUILDDIR=$LOCALBUILDDIR/$LIBNAME-$TRIPLET
}

do_clean_get() {
  do_clean $1
  do_download

  if [[ ! -d "$LIBBUILDDIR" ]]; then
    mkdir "$LIBBUILDDIR"
  fi
  cd "$LIBBUILDDIR"
}


PATH_CHANGE_REV_FILENAME=".last_success_revision"

#hash a dir based on the git revision, $TRIPLET and $tools 
#param1 path to be hashed
function getBuildHash ()
{
  local checkPath
  checkPath="$1"
  shift 1
  local hashStr
  hashStr="$(git rev-list HEAD --max-count=1  -- $checkPath $@)"
  hashStr="$hashStr $@ $TRIPLET $TOOLS"
  echo $hashStr
}

#param1 path to be checked for changes
function pathChanged ()
{
  local ret
  local checkPath
  ret="0"
  #no optims in release builds!
  if [ "$Configuration" == "Release" ]
  then
    echo "1"
    return
  fi

  checkPath="$1"
  shift 1
  if [ -e $checkPath/$PATH_CHANGE_REV_FILENAME ]
  then
    if [ "$(cat $checkPath/$PATH_CHANGE_REV_FILENAME)" != "$(getBuildHash $checkPath $@)" ]
    then
      ret="1"
    fi
  else
    ret="1"
  fi
  
  echo $ret
}

#param1 path to be tagged with hash
function tagSuccessFulBuild ()
{
  local pathToTag
  pathToTag="$1"
  shift 1
  # tag last successful build with revisions of the given dir
  # needs to match the checks in function getBuildHash
  echo "$(getBuildHash $pathToTag $@)" > $pathToTag/$PATH_CHANGE_REV_FILENAME
}
