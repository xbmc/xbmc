#!/bin/bash

releaseversion=${VERSION:-"AUTO"}
epoch=${EPOCH:-"2"}
gitrev=${GITREV:-"$(git log -1 --pretty=format:"%h")"}
tag=${TAG:-${gitrev}}
tagrev=${tagrev:-"0"}
dists=${DISTS:-"lunar kinetic jammy focal"}
gpgkey=${GPG_KEY:-"jenkins (jenkins build bot) <jenkins@kodi.tv>"}
ppa=${PPA:-"nightly"}
debianrepo="${DEBIAN:-"https://github.com/xbmc/xbmc-packaging"}"

if [ "$releaseversion" = "AUTO" ]; then
  majorversion="$(awk '/VERSION_MAJOR/ {print $2}' version.txt)"
  minorversion="$(awk '/VERSION_MINOR/ {print $2}' version.txt)"
  releaseversion="${majorversion}.${minorversion}"
fi

version="${releaseversion}+git$(date '+%Y%m%d.%H%M')-${tag}"
debversion="${epoch}:${version}"
origtarball="kodi_${version}.orig.tar.gz"

declare -A PPAS=(
    ["nightly"]='ppa:team-xbmc/xbmc-nightly'
    ["unstable"]='ppa:team-xbmc/unstable'
    ["stable"]='ppa:team-xbmc/ppa'
    ["wsnipex-nightly"]='ppa:wsnipex/kodi-git'
    ["wsnipex-stable"]='ppa:wsnipex/kodi-stable'
)

# clean up before creating the source tarball
git clean -xfd

# set build info
date '+%Y%m%d' > BUILDDATE
echo $gitrev > VERSION

# download packaging files
wget -O - ${debianrepo}/archive/master.tar.gz | tar xzv --strip-components=1 --exclude=".git*" -f -
[ -d debian ] || { echo "ERROR: directory debian does not exist"; exit 3; }

# add tarballs for internal ffmpeg, libdvd
make -C tools/depends/target/ffmpeg download || { echo "Error downloading ffmpeg"; exit 2; }
make -C tools/depends/target/libdvdnav download || { echo "Error downloading libdvdnav"; exit 2; }
make -C tools/depends/target/libdvdread download || { echo "Error downloading libdvdread"; exit 2; }
make -C tools/depends/target/libdvdcss download || { echo "Error downloading libdvdcss"; exit 2; }
make -C tools/depends/target/dav1d download || { echo "Error downloading dav1d"; exit 2; }

# create orig tarball if needed
if grep -q quilt debian/source/format; then
  echo "origtarball: ${origtarball}"
  git archive -o ../${origtarball} ${gitrev}
fi


# build source packages
for dist in ${dists//,/ }; do
  echo "### Building for ${dist} ###"
  sed \
      -e "s/#PACKAGEVERSION#/${debversion}/" \
      -e "s/#TAGREV#/${tagrev}/" \
      -e "s/#DIST#/${dist}/g" \
    debian/changelog.in > debian/changelog

  echo "Changelog:"
  cat debian/changelog
  echo

  debuild -d -S -k"${gpgkey}"
  echo "### DONE ###"
done

# upload to PPA
echo "### Uploading to PPA ${PPAS[${ppa}]} ###"
dput ${PPAS[${ppa}]} ../kodi_${version}*.changes
if [ $? -eq 0 ]; then
  echo "### Successfully pushed ${version} to launchpad ###"
else
  echo "### ERROR could not upload package ###"
fi

