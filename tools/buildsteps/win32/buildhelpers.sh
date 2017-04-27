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

do_wget() {
  local URL="$1"
  local archive="$2"

  if [[ -z $archive ]]; then
    wget --tries=5 --retry-connrefused --waitretry=2 --no-check-certificate -c $URL
  else
    wget --tries=5 --retry-connrefused --waitretry=2 --no-check-certificate -c $URL -O $archive
  fi
}

do_makeinstall() {
  make -j"$cpuCount" "$@"
  make install
}

do_makelib() {
  do_print_status "$LIBNAME-$VERSION (${BITS})" "$blue_color" "Compiling"
  do_makeinstall $1
  do_print_status "$LIBNAME-$VERSION (${BITS})" "$green_color" "Done"
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

get_last_version() {
  local filelist="$1"
  local filter="$2"
  local version="$3"
  local ret=$(echo "$filelist" | /usr/bin/grep -E "$filter" | sort -V | tail -1)
  if [[ -z "$version" ]]; then
    echo $ret
  else
    echo $ret | /usr/bin/grep -oP "$version"
  fi
}

do_autoreconf() {
  if [[ ! -f configure ]]; then 
    autoreconf -fiv
  fi
}

do_clean() {
  if [[ "$1" == "clean" ]] || [[ ! -f $ARCHIVE ]]; then
    if [ -d $LIBNAME ]; then
      do_print_status "$LIBNAME" "$red_color" "Removing"
      rm -r $LIBNAME
    fi
  fi
}

do_download() {
  if [ ! -d $LIBNAME ]; then
    if [ ! -f $ARCHIVE ]; then
      do_print_status "$ARCHIVE" "$orange_color" "Downloading"
      do_wget $BASE_URL/$VERSION.tar.gz $ARCHIVE
    fi

    do_print_status "$ARCHIVE" "$blue_color" "Extracting"
    mkdir $LIBNAME && cd $LIBNAME
    tar -xaf ../$ARCHIVE --strip 1
  else
    cd $LIBNAME
  fi
}

do_loaddeps() {
  local file="$1"
  LIBNAME=$(grep "LIBNAME=" $file | sed 's/LIBNAME=//g;s/#.*$//g;/^$/d')
  BASE_URL=$(grep "BASE_URL=" $file | sed 's/BASE_URL=//g;s/#.*$//g;/^$/d')
  VERSION=$(grep "VERSION=" $file | sed 's/VERSION=//g;s/#.*$//g;/^$/d')
  GNUTLS_VER=$(grep "GNUTLS_VER=" $file | sed 's/GNUTLS_VER=//g;s/#.*$//g;/^$/d')
  GITREV=$(git ls-remote $BASE_URL $VERSION | awk '{print $1}')
  BASE_URL=$BASE_URL/archive
  if [[ -z "$GITREV" ]]; then
    ARCHIVE=$LIBNAME-$(echo "${VERSION}" | sed 's/\//-/g').tar.gz
  else
    ARCHIVE=$LIBNAME-$GITREV.tar.gz
  fi
}

do_clean_get() {
  cd "$LOCALBUILDDIR"
  do_clean $1
  do_download
}


PATH_CHANGE_REV_FILENAME=".last_success_revision"

#hash a dir based on the git revision, $BITS and $tools 
#param1 path to be hashed
function getBuildHash ()
{
  local checkPath
  checkPath="$1"
  shift 1
  local hashStr
  hashStr="$(git rev-list HEAD --max-count=1  -- $checkPath $@)"
  hashStr="$hashStr $@ $BITS $tools"
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
