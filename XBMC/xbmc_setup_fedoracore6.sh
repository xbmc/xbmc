#!/bin/sh
#
# $Header$
# Unix shell script to setup Fedora Core 6 for development of the 
# Linux port of XBMC

# note, the glew packages are from the 'dribble' repository
# to setup the livna repo... 
#     rpm -ivh http://rpm.livna.org/livna-release-6.rpm

# to setup the dribble repo...
#     rpm -ivh http://dribble.org.uk/repo/dribble-release-5-3.noarch.rpm

# list of all required packages
reqd_pkgs=( \
   gcc               \
   gcc-c++           \
   SDL               \
   SDL-devel         \
   SDL_image         \
   SDL_image-devel   \
   SDL_gfx           \
   SDL_gfx-devel     \
   SDL_mixer         \
   SDL_mixer-devel   \
   SDL_sound         \
   SDL_sound-devel   \
   libcdio           \
   libcdio-devel     \
   fribidi           \
   fribidi-devel     \
   lzo               \
   lzo-devel         \
   freetype          \
   freetype-devel    \
   sqlite            \
   sqlite-devel      \
   libogg            \
   libogg-devel      \
   samba-client      \
   python            \
   python-devel      \
   python-sqlite     \
   curl              \
   curl-devel        \
   glew              \
   glew-devel        \
   alsa-lib-devel    \
   alsa-lib          \
)


len=${#reqd_pkgs[*]}
echo "Checking to see if the $len pre-reqs are installed:"
i=0
while [ $i -lt $len ]; do
        # check to see if it's installed
        rpm -q "${reqd_pkgs[$i]}"
        if [ $? -ne 0 ]
           then
              echo " ${reqd_pkgs[$i]} will be installed"
              yum install -y "${reqd_pkgs[$i]}"
           fi
	let i++
done



