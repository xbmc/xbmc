#!/bin/bash

usage()
{
myEcho "For --tag|-t, use the following options:
  svnsrc=<dir>
  version=<version> (without 'xbmc-')
  "
myEcho "For -u, use:
  srcdir=<dir>
  version=<version> (or revision)
  minor=<minor>
  "
myEcho "For -nsg, use:
  srcdir=<dir>
  revision=<revision> (or version=<version>)
  "
myEcho "For -prev, use:
  rev=<revision>
  "
myEcho "You can use:
  > -debian-tag=<tag> (to select the debian directory tag to use)
  "
exit 0
}

myEcho() {
  echo -e "$1" 
  echo -e "$1" >> $BUILD_LOG
}

BUILDALL=1
FULLDEBUILDOPTS="-S -sa"
DEBUILDOPTS="-S -sd"
MINOR=1
XBMCPPA=xbmc-svn
HVERSION=9.11~beta1
DEBIAN_TAG_OPT=
BUILD_LOG=debuilder_`date +%F_%T`.log


export DEBFULLNAME="Ouattara Oumar Aziz (alias wattazoum)"
export DEBEMAIL="wattazoum@gmail.com"

parse_options()
{
  for I in "$@"
  do
    OPT=${I%=*}
    PAR=${I#*=}
    case $OPT in
      --local|-l)
        LOCAL=1
      ;;
      --pbuilder|-p)
        PBUILDER=1
        LOCAL=1
      ;;
      hardy|intrepid|jaunty|karmic)
        BUILDALL=0
        DIST="$OPT"
        DEBUILDOPTS=$FULLDEBUILDOPTS
      ;;
      --noclean|-nc)
        NOCLEAN=1
      ;;
      --no-src-gen|-nsg)
        NO_SRC_GEN=1
      ;;
      --update|-u)
        UPDPPA=1
        FULLDEBUILDOPTS=$DEBUILDOPTS
      ;;
      --tag|-t)
        BUILD_TAG=1
      ;;
      -debian-tag)
        DEBIAN_TAG_OPT="-r $PAR"
      ;;
      -prev)
        EXPORT_PREV_REV=1
      ;;
      --help|-h)
        usage
      ;;
      *)
        myEcho "Setting $OPT=$PAR"
        export $OPT=$PAR
      ;;
    esac
  done
}

getrootright()
{
  if [[ $PBUILDER ]]; then 
    myEcho "Give me the admin rights ... "
    sudo echo "Thank you !"
  fi
}

preparesrc() 
{
  myEcho "Exporting the sources at revision $REVISION ... "
  if [[ -z $HEAD_REVISION ]]; then
    # The revision given might not be the head one
    svn export -r $REVISION $SVNSRC $DESTSRC 2>&1
  else
    svn cleanup $SVNSRC 
    svn export $SVNSRC $DESTSRC 
  fi
  myEcho "Copying to .orig folder"
  cp -a $DESTSRC $DESTSRC.orig
}

builddeb()
{
  myEcho "Copy the debian folder to the root"
  cp -a $DESTSRC/tools/Linux/packaging/debian $DESTSRC/debian

  if [[ -z $CHNLG ]]; then
    CHNLG="Build of $VERSION"
  fi
  myEcho "Changelog : $CHNLG"
  cd $DESTSRC
  dch -v ${VERSION}-$1${MINOR} -D $1 "$CHNLG" 2>&1
  myEcho "$REVISION" > debian/svnrevision
  myEcho "Building the $1 debian package" 

  if [ $1 -eq "hardy"] ; then
    tweaks_for_hardy
  fi

  # Add vdpau dependencies
  mv debian/control debian/control.orig
  sed s/"make,"/"make, nvidia-190-libvdpau-dev,"/ debian/control.orig > debian/control
  rm debian/control.orig

  if [[ $BUILT_ONCE ]]; then
    debuild $DEBUILDOPTS 2>&1
  else 
    debuild $FULLDEBUILDOPTS  2>&1
    BUILT_ONCE=1
  fi
  
  cd $OLDPWD
  if [[ $PBUILDER ]]; then 
    myEcho "'pbuilder' is set. Trying into pbuilder"
    $SCRIPTDIR/pbuilder-dist $1 build xbmc_${VERSION}-$1${MINOR}.dsc 2>&1
    rm -rf $DESTSRC/debian
  fi
  if [[ -z $LOCAL ]]; then 
    myEcho "'--local' is not set. Uploading to PPA"
    dput $XBMCPPA xbmc_${VERSION}-$1${MINOR}_source.changes 2>&1
    rm -rf $DESTSRC/debian
  fi
}

tweaks_for_hardy() 
{
  # change debhelper version in control
  mv debian/control debian/control.orig
  sed -r s/"debhelper \(>= .+?\)"/"debhelper (>= 7)"/ debian/control.orig > debian/control
  rm debian/control.orig
 
  # change the rules file.
  rm -f debian/rules
  cp debian/rules.hardy debian/rules
  # move the format spec to 1.0
  echo "1.0" > debian/source/format
}

clean()
{
if [[ -z $NOCLEAN ]] && [[ -z $LOCAL ]] ; then
  myEcho "Cleaning ... "
  find . -depth -maxdepth 1 -regex "\./xbmc[-_].+-.+" -not -name "*.orig.tar.gz" -exec rm -rf {} \;
  if [[ -z $UPDPPA ]] && [[ -z $NO_SRC_GEN ]] ; then
    rm -rf xbmc-$VERSION xbmc_$VERSION.orig.tar.gz
  fi
fi
}

preparevars()
{
  myEcho "Preparing Vars ..."
  echo "Nothing" > test.txt
  gpg -s test.txt
  rm test.txt test.txt.gpg
  
  BUILD_LOG=$BUILD_DIR/debuilder_`date +%F_%T`.log
  myEcho "Build directory: $BUILD_DIR"

  if [[ -z $SVNSRC ]]; then
    SVNSRC=$(readlink -f ../../../)
  fi
  if [[ $UPDPPA ]]; then
    DESTSRC=$srcdir
    if [[ $revision  ]]; then
      REVISION=$revision
      VERSION=${HVERSION}+svn$REVISION
    fi
    if [[ -z $VERSION ]] && [[ $version ]]; then
      VERSION=$version
    fi
    MINOR=$minor
    BUILT_ONCE=1
  fi
  if [[ $NO_SRC_GEN ]]; then
    DESTSRC=$srcdir
    if [[ $revision  ]]; then
      REVISION=$revision
      VERSION=${HVERSION}+svn$REVISION
    fi
    if [[ -z $VERSION ]] && [[ $version ]]; then
      VERSION=$version
    fi
  fi
  if [[ $BUILD_TAG ]]; then
    VERSION=$version
  fi
  if [[ $EXPORT_PREV_REV ]]; then
    myEcho "Revision to export: $rev"
    REVISION=$rev
  fi

  myEcho "Setting SVN Sources: $SVNSRC"
  
  # If the version is not yet set it
  if [ -z $REVISION ]; then
    svn update $SVNSRC
    HEAD_REVISION=$(expr $(svn info $SVNSRC --xml 2>&1 | grep -m 1 -e "revision=\"[0-9]*\">") : 'revision="\([0-9]\+\)">')
    REVISION=$HEAD_REVISION
  fi
  if [ -z $VERSION ]; then
    VERSION=${HVERSION}+svn$REVISION
  fi

  myEcho "XBMC version: $VERSION"
  myEcho "XBMC revision: $REVISION"

  # Set Destination folder if not set
  if [ -z $DESTSRC ]; then
    DESTSRC=xbmc-$VERSION
  fi

  myEcho "XBMC Destination folder: $DESTSRC"
  myEcho "Package minor version: $MINOR"

}

# = = = = = = = = = = = = = = 

SCRIPTDIR=`pwd`

# We are in the source tree. Go out
if [ -z $BUILD_DIR ] ; then
  BUILD_DIR=$(eval readlink -f ../../../../)
fi

parse_options
getrootright
preparevars

cd $BUILD_DIR

if [[ -z $UPDPPA ]] && [[ -z $NO_SRC_GEN ]] ; then
  preparesrc
fi

for distro in "hardy" "intrepid" "jaunty" "karmic"; do 
  if [ $BUILDALL -eq 1 ] || [ $DIST == "$distro" ]; then
    builddeb $distro
  fi
done

clean

exit 0
