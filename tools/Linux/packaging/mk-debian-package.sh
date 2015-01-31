#!/bin/bash
#
# Copyright (C) 2013 Team XBMC
# http://kodi.tv
#
# This Program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This Program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with XBMC; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#


RELEASEV=${RELEASEV:-"auto"}
VERSION_PREFIX=${VERSION_PREFIX:-""}
TAG=${TAG}
REPO_DIR=${WORKSPACE:-$(cd "$(dirname $0)/../../../" ; pwd)}
[[ $(which lsb_release) ]] && DISTS=${DISTS:-$(lsb_release -cs)} || DISTS=${DISTS:-"stable"}
ARCHS=${ARCHS:-$(dpkg --print-architecture)}
BUILDER=${BUILDER:-"debuild"}
DEBUILD_OPTS=${DEBUILD_OPTS:-""}
PDEBUILD_OPTS=${PDEBUILD_OPTS:-""}
PBUILDER_BASE=${PBUILDER_BASE:-"/var/cache/pbuilder"}
DPUT_TARGET=${DPUT_TARGET:-"local"}
DEBIAN=${DEBIAN:-"https://github.com/xbmc/xbmc-packaging/archive/master.tar.gz"}
BUILD_DATE=$(date '+%Y%m%d.%H%M')

function usage {
    echo "$0: this script builds a Kodi debian package from a git repository."
    echo "The build is controlled by ENV variables, which van be overridden as appropriate:"
    echo "BUILDER is either debuild(default) or pdebuild(needs a proper pbuilder setup)"
    checkEnv
}

function checkEnv {
    echo "#------ build environment ------#"
    echo "REPO_DIR: $REPO_DIR"
    getVersion
    echo "RELEASEV: $RELEASEV"
    [[ -n $TAG ]] && echo "TAG: $TAG"
    echo "DISTS: $DISTS"
    echo "ARCHS: $ARCHS"
    echo "DEBIAN: $DEBIAN"
    echo "BUILDER: $BUILDER"
    echo "CONFIGURATION: $Configuration"

    if ! [[ $(which $BUILDER) ]]
    then
        echo "Error: can't find ${BUILDER}, consider using full path to [debuild|pdebuild]"
        exit 1
    fi

    if [[ "$BUILDER" =~ "pdebuild" ]]
    then
        if ! [[ -d $PBUILDER_BASE ]] ; then echo "Error: $PBUILDER_BASE does not exist"; exit 1; fi
        echo "PBUILDER_BASE: $PBUILDER_BASE"
        echo "PDEBUILD_OPTS: $PDEBUILD_OPTS"
    else
        echo "DEBUILD_OPTS: $DEBUILD_OPTS"
    fi

    echo "#-------------------------------#"
}

function getVersion {
    getGitRev
    if [[ $RELEASEV == "auto" ]]
    then 
        local MAJORVER=$(grep VERSION_MAJOR $REPO_DIR/version.txt | awk '{ print $2 }')
        local MINORVER=$(grep VERSION_MINOR $REPO_DIR/version.txt | awk '{ print $2 }')
        RELEASEV=${MAJORVER}.${MINORVER}
    else
        PACKAGEVERSION="${RELEASEV}~git${BUILD_DATE}-${TAG}"
    fi

    if [[ -n ${VERSION_PREFIX} ]]
    then
        PACKAGEVERSION="${VERSION_PREFIX}:${RELEASEV}~git${BUILD_DATE}-${TAG}"
    else
        PACKAGEVERSION="${RELEASEV}~git${BUILD_DATE}-${TAG}"
    fi
}

function getGitRev {
    cd $REPO_DIR || exit 1
    REV=$(git log -1 --pretty=format:"%h")
    [[ -z $TAG ]] && TAG=$REV
    [[ -z $TAGREV ]] && TAGREV=0
}

function archiveRepo {
    cd $REPO_DIR || exit 1
    git clean -xfd
    echo $REV > VERSION
    tools/depends/target/ffmpeg/autobuild.sh -d
    DEST="kodi-${RELEASEV}~git${BUILD_DATE}-${TAG}"
    [[ -d debian ]] && rm -rf debian
    cd ..
    tar -czf ${DEST}.tar.gz -h --exclude .git $(basename $REPO_DIR)
    ln -s ${DEST}.tar.gz ${DEST/-/_}.orig.tar.gz
    echo "Output Archive: ${DEST}.tar.gz"

    cd $REPO_DIR || exit 1
    getDebian
}

function getDebian {
    if [[ -d $DEBIAN ]]
    then
        cp -r $DEBIAN .
    else
        mkdir tmp && cd tmp
        curl -L -s $DEBIAN -o debian.tar.gz
        tar xzf debian.tar.gz
        cd xbmc-packaging-*
        for FILE in *.unified; do mv $FILE debian/${FILE%.unified}; done
        mv debian $REPO_DIR
        cd $REPO_DIR ; rm -rf tmp
    fi
}

function buildDebianPackages {
    archiveRepo
    cd $REPO_DIR || exit 1
    sed -e "s/#PACKAGEVERSION#/${PACKAGEVERSION}/g" -e "s/#TAGREV#/${TAGREV}/g" debian/changelog.in > debian/changelog.tmp
    [ "$Configuration" == "Debug" ] && sed -i "s/XBMC_RELEASE = yes/XBMC_RELEASE = no/" debian/rules

    for dist in $DISTS
    do
        sed "s/#DIST#/${dist}/g" debian/changelog.tmp > debian/changelog
        for arch in $ARCHS
        do
            cd $REPO_DIR
            echo "building: DIST=$dist ARCH=$arch"
            if [[ "$BUILDER" =~ "pdebuild" ]]
            then
                DIST=$dist ARCH=$arch $BUILDER $PDEBUILD_OPTS
                [ $? -eq 0 ] && uploadPkg || exit 1
            else
                $BUILDER $DEBUILD_OPTS
                echo "output directory: $REPO_DIR/.."
            fi
        done
    done
}

function uploadPkg {
    PKG="${PBUILDER_BASE}/${dist}-${arch}/result/${DEST/-/_}-${TAGREV}_${arch}.changes"
    echo "signing package"
    debsign $PKG
    echo "uploading $PKG to $DPUT_TARGET"
    dput $DPUT_TARGET $PKG
    UPLOAD_DONE=$?
}

function cleanup {
    if [[ $UPLOAD_DONE -eq 0 ]] && [[ "$BUILDER" =~ "pdebuild" ]]
    then
        cd $REPO_DIR/.. || exit 1
        rm ${DEST}*
        rm ${DEST/-/_}*
    fi
}

###
# main
###
if [[ $1 = "-h" ]] || [[ $1 = "--help" ]]
then
    usage
    exit
fi

checkEnv
buildDebianPackages
cleanup

