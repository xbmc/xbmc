#!/bin/bash

usage()
{
echo "
  This script requires debhelper, pbuilder, dput:
  $ sudo apt-get install debhelper pbuilder dput subversion

  The following options are supported: 
	--no-export	: Ask the script to no do an SVN export (do a copy instead)
	--ppa=<your ppa in dput.cf>
	-t, --tag 	: svnsrc=<dir>, version=<version> (without 'xbmc-')
	-u		: srcdir=<dir> version=<version> revision=<rev> minor=<minor>
	-nsg		: svnsrc=<dir> revision=<rev>
	-prev		: revision=<rev> 
	urgency=(low|medium|high)
"
exit 0
}

alias echo='echo -e'

BUILDALL=1
FULLDEBUILDOPTS="-S -sa"
DEBUILDOPTS="-S -sd"
MINOR=1
XBMCPPA=xbmc-svn
# Information about versioning
# SVN before releasing version NN is: NN~svnXXXX
# SVN after releasing version NN is: NN+svnXXXX
SVN_RELEASE_SEP="~"
HVERSION=10.08

# Packagers should have these two vars in their environment
# export DEBFULLNAME="Ouattara Oumar Aziz (alias wattazoum)"
# export DEBEMAIL="wattazoum@gmail.com"

parse_options()
{
  echo "Parse options: $@"

  for I in "$@"
  do
    OPT=${I%=*}
    PAR=${I#*=}
    case $OPT in
      --local|-l)
        LOCAL=1
      ;;
      --pbuilder|-p)
        echo "-p ==> PBUILDER"	
        PBUILDER=1
        LOCAL=1
      ;;
      hardy|intrepid|jaunty|karmic|lucid)
        BUILDALL=0
        DIST="$OPT"
        DEBUILDOPTS=$FULLDEBUILDOPTS
        echo "DIST ==> $DIST"
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
      -prev)
        EXPORT_PREV_REV=1
      ;;
      --ppa)
        XBMCPPA=$PAR
      ;;
      --no-export)
        NO_EXPORT=1
      ;;
      --help|-h)
        usage
      ;;
      *)
        echo "Setting $OPT=$PAR"
        export $OPT=$PAR
      ;;
    esac
  done
}

getrootright()
{
  if [[ $PBUILDER ]]; then 
    echo "Give me the admin rights ... "
    sudo echo "Thank you !"
  fi
}

preparesrc() 
{
  echo "Exporting the sources at revision $REVISION ... "
  if [[ -z $NO_EXPORT ]]; then 
    if [[ -z $HEAD_REVISION ]]; then
      # The revision given might not be the head one
      svn export -r $REVISION $SVNSRC $DESTSRC 2>&1 
    else
      svn cleanup $SVNSRC 
      svn export $SVNSRC $DESTSRC 
    fi
  else
    cp $SVNSRC $DESTSRC -Rf
  fi
  cd $DESTSRC
  ./bootstrap
  cd $OLDPWD
  if [ -z $UPDPPA ]; then
    echo "Copying to .orig folder"
    cp -a $DESTSRC $DESTSRC.orig
  fi
}

builddeb()
{
  echo "Build $1 deb package"
  echo "Copy the debian folder to the root"
  cp -a $DESTSRC/tools/Linux/packaging/debian $DESTSRC/debian

  if [[ -z $CHNLG ]]; then
    CHNLG="Build of $VERSION"
  fi
  echo "Changelog : $CHNLG"
  cd $DESTSRC
  if [[ -z $urgency ]]; then
    urgency=low
  fi
  if [ $UPDPPA ]; then
    PKG_VERSION=1:${HVERSION}-$1$REVISION
  else
    PKG_VERSION=1:${VERSION}-$1${MINOR}
  fi
  dch --force-distribution -b -v $PKG_VERSION -D $1 -u $urgency "$CHNLG" 2>&1 
  echo "$REVISION" > debian/svnrevision
  echo "Building the $1 debian package" 
  
  echo "move the format spec to 1.0 (Ubuntu PPA doesn't support format 3.0 quilt) "
  echo "1.0" > debian/source/format

  if [[ $BUILT_ONCE ]]; then
    debuild $DEBUILDOPTS 2>&1
  else 
    debuild $FULLDEBUILDOPTS  2>&1
    BUILT_ONCE=1
  fi
  
  cd $OLDPWD
  if [[ $PBUILDER ]]; then 
    echo "'pbuilder' is set. Trying into pbuilder"
    $SCRIPTDIR/pbuilder-dist $1 build xbmc_${VERSION}-$1${MINOR}.dsc 2>&1 | tee -a $BUILD_LOG
    rm -rf $DESTSRC/debian
  fi
  if [[ -z $LOCAL ]]; then 
    echo "'--local' is not set. Uploading to PPA"
    dput $XBMCPPA xbmc_${VERSION}-$1${MINOR}_source.changes 2>&1
    rm -rf $DESTSRC/debian
  fi
}

clean()
{
if [[ -z $NOCLEAN ]] && [[ -z $LOCAL ]] ; then
  echo "Cleaning ... "
  find . -depth -maxdepth 1 -regex "\./xbmc[-_].+-.+" -not -name "*.orig.tar.gz" -exec rm -rf {} \;
  if [[ -z $UPDPPA ]] && [[ -z $NO_SRC_GEN ]] ; then
    rm -rf xbmc-$VERSION xbmc_$VERSION.orig.tar.gz
  fi
fi
}

preparevars()
{
  echo "Nothing" > test.txt
  gpg -s test.txt
  rm test.txt test.txt.gpg
  
  echo "Preparing Vars ..."
  echo "Build directory: $BUILD_DIR"

  if [[ -z $SVNSRC ]]; then
    SVNSRC=$(readlink -f ../../../)
  fi
  if [[ $UPDPPA ]]; then
    DESTSRC=$srcdir
    if [[ $revision  ]]; then
      REVISION=$revision
      VERSION=${HVERSION}${SVN_RELEASE_SEP}svn$REVISION
    fi
    if [[ -z $VERSION ]] && [[ $version ]]; then
      VERSION=$version
    fi
    if [ $minor ]; then
      MINOR=$minor
    fi
    BUILT_ONCE=1
  fi
  if [[ $NO_SRC_GEN ]]; then
    DESTSRC=$srcdir
    if [[ $revision  ]]; then
      REVISION=$revision
      VERSION=${HVERSION}${SVN_RELEASE_SEP}svn$REVISION
    fi
    if [[ -z $VERSION ]] && [[ $version ]]; then
      VERSION=$version
    fi
  fi
  if [[ $BUILD_TAG ]]; then
    VERSION=$version
  fi
  if [[ $EXPORT_PREV_REV ]]; then
    echo "Revision to export: $rev"
    REVISION=$rev
  fi

  echo "Setting SVN Sources: $SVNSRC"
  
  # If the version is not yet set it
  if [[ -z $REVISION ]]; then
    svn update $SVNSRC
    HEAD_REVISION=$(eval LANG=POSIX svn info $SVNSRC | grep -E "Last Changed Rev: [0-9]+" | grep -o -E "[0-9]+")
    REVISION=$HEAD_REVISION
  fi
  if [[ -z $VERSION ]]; then
    VERSION=${HVERSION}${SVN_RELEASE_SEP}svn$REVISION
  fi

  echo "XBMC version: $VERSION"
  echo "XBMC revision: $REVISION"

  # Set Destination folder if not set
  if [[ -z $DESTSRC ]]; then
    DESTSRC=xbmc-$VERSION
  fi

  echo "XBMC Destination folder: $DESTSRC"
  echo "Package minor version: $MINOR"

}

# = = = = = = = = = = = = = = 

SCRIPTDIR=`pwd`

if [[ -z $BUILD_DIR ]] ; then
  BUILD_DIR=$(eval readlink -f ../../../../)
fi

BUILD_LOG=$BUILD_DIR/debuilder_`date +%F_%T`.log

parse_options $@ 
getrootright
preparevars

# We are in the source tree. Go out
cd $BUILD_DIR

if [[ -z $NO_SRC_GEN ]] ; then
  preparesrc
fi

for distro in hardy jaunty karmic lucid ; do 
  if [[ $BUILDALL -eq 1 ]] || [[ $DIST == $distro ]]; then
    builddeb $distro
  fi
done

clean

exit 0
