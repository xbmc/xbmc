#!/bin/bash

# This script is for purely testing purposes!
#
# It is meant to be able to test at binary addons the creation of a debian
# package.
#
# The created files are below the source code folder.
#
# Example:
#   ./build-debian-addon-package.sh $HOME/your_path/screensaver.stars
#
# To remove generated code:
#   ./build-debian-addon-package.sh $HOME/your_path/screensaver.stars --clean
#

BASE_DIR=""
REPO_DIR=""
PACKAGEVERSION=""
VERSION_MAIN=""
VERSION_MINOR=""
VERSION_REVISON=""
GIT_REV=""
DIST=""
TAGREV=${TAGREV:-"0"}

function usage {
    echo "\
--------------------------------------------------------------------------------

This script builds a Kodi addon debian package from the given addon source.

As value, the path to the addon must be given.
In addition --clean can be used to remove the created debian files.

WARNING: This script is for testing purposes only!

--------------------------------------------------------------------------------"
}

function checkEnv {
  echo "#------ build environment ------#"
  echo "BASE_DIR: $BASE_DIR"
  echo "REPO_DIR: $REPO_DIR"
  getVersion
  echo "VERSION_MAIN: $VERSION_MAIN"
  echo "VERSION_MINOR: $VERSION_MINOR"
  echo "VERSION_REVISON: $VERSION_REVISON"
  if [ $GIT_REV ]; then
    echo "GIT_REV: $GIT_REV"
  fi
  echo "TAGREV: $TAGREV"
  echo "DIST: $DIST"
  echo "ARCHS: $ARCHS"

  echo "#-------------------------------#"
}

function getVersion {
  if [ -d ${BASE_DIR}/.git ]; then
    getGitRev
  fi
  PACKAGEVERSION=$(cat ${BASE_DIR}/$REPO_DIR/addon.xml.in | sed -n '/version/ s/.*version=\"\([0-9]\+\.[0-9]\+\.[0-9]\+\)\".*/\1/p' | awk 'NR == 1')
  VERSION_MAIN=$(echo $PACKAGEVERSION | awk -F. '{print $1}')
  VERSION_MINOR=$(echo $PACKAGEVERSION | awk -F. '{print $2}')
  VERSION_REVISON=$(echo $PACKAGEVERSION | awk -F. '{print $3}')
}

function getGitRev {
    cd $BASE_DIR || exit 1
    GIT_REV=$(git log -1 --pretty=format:"%h")
}

function cleanup() {
  echo "Starting to remove debian generated files"
  cd ${BASE_DIR}
  rm -rf obj-$ARCHS-linux-gnu
  rm -rf debian/.debhelper
  rm -rf debian/"kodi-"$(echo $REPO_DIR | tr . -)
  rm -rf debian/"kodi-"$(echo $REPO_DIR | tr . -)-dbg
  rm -rf debian/tmp
  rm -f debian/changelog
  rm -f debian/debhelper-build-stamp
  rm -f debian/files
  rm -f debian/*.log
  rm -f debian/*.substvars
}

if [[ $1 = "-h" ]] || [[ $1 = "--help" ]]; then
  echo "$0:"
  usage
  exit
fi

if [ ! $1 ] || [ ${1} = "--clean" ]; then
  printf "$0:\nERROR: Addon source code must be given as the first parameter!\n"
  usage
  exit 1
elif [ ! -d $1 ]; then
  printf "$0:\nERROR: Given folder is not present or not a valid addon source!\n"
  usage
  exit 1
fi

ARCHS=$(uname -m)
BASE_DIR=$(realpath ${1})
REPO_DIR=$(basename ${1})
DIST=$(lsb_release -cs)

if [ ! -f ${BASE_DIR}/$REPO_DIR/addon.xml.in ]; then
  echo "$0:
ERROR: \"required $REPO_DIR/addon.xml.in\" not found!

The base source dir and addon.xml.in containing directory names must be equal"
  usage
  exit 1
fi

checkEnv

ORIGTARBALL="kodi-"$(echo $REPO_DIR | tr . -)"_${PACKAGEVERSION}.orig.tar.gz"

if [[ ${2} = "--clean" ]]; then
  cleanup
  exit
fi

echo "Detected addon package version: ${PACKAGEVERSION}"

sed -e "s/#PACKAGEVERSION#/${PACKAGEVERSION}/g" \
    -e "s/#TAGREV#/${TAGREV}/g" \
    -e "s/#DIST#/${DIST}/g" ${BASE_DIR}/debian/changelog.in > ${BASE_DIR}/debian/changelog

echo "Create needed compressed source code file: ${BASE_DIR}/../${ORIGTARBALL}.tar.gz"

# git archive --format=tar.gz -o ${BASE_DIR}/../${ORIGTARBALL} HEAD # Unused git, leaved as optional way
EXCLUDE=$(cat .gitignore | sed -e '/#/d' -e '/^\s*$/d' -e 's/^/--exclude=/' -e 's/\/$//' | tr '\n' ' ')
tar -zcvf ${BASE_DIR}/../${ORIGTARBALL} --exclude=.git $EXCLUDE * # Used to prevent on code changed for test a git commit

echo "Building debian-source package for ${DIST}"
dpkg-buildpackage -us -uc --diff-ignore="()$"
