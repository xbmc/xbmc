#!/bin/sh

# A simple script that will automatically download a predefined list of packages,
# skipping over those already downloaded, and then extracting them to prefix folder /usr

# There are two different addresses to get different packages from
PACKAGES_ADDR=ftp.uk.debian.org/debian/pool/main
SECURITY_ADDR=security.debian.org/debian-security/pool/updates/main

# This is the list of package addresses. This list cannot be smaller than the size of DEB_FILE,
# but can be bigger - anything over is ignored.
# This must be in the same order as DEB_FILE aswell!
DEB_ADDR=(
$PACKAGES_ADDR/b/boost
$PACKAGES_ADDR/libm/libmad
$PACKAGES_ADDR/libv/libvorbis
$PACKAGES_ADDR/f/fribidi
$PACKAGES_ADDR/m/mysql-dfsg-5.0
$PACKAGES_ADDR/p/pcre3
$PACKAGES_ADDR/l/lzo2
$PACKAGES_ADDR/l/lzo2
$PACKAGES_ADDR/libs/libsdl1.2
$PACKAGES_ADDR/libs/libsdl1.2
$PACKAGES_ADDR/libs/libsdl1.2
$PACKAGES_ADDR/s/sdlgfx
$PACKAGES_ADDR/s/sdlgfx
$PACKAGES_ADDR/s/sdl-image1.2
$PACKAGES_ADDR/s/sdl-image1.2
$PACKAGES_ADDR/t/tiff
$PACKAGES_ADDR/s/sdl-mixer1.2
$PACKAGES_ADDR/s/sdl-mixer1.2
$PACKAGES_ADDR/libm/libmikmod
$PACKAGES_ADDR/s/smpeg
$PACKAGES_ADDR/libv/libvorbis
$PACKAGES_ADDR/libv/libvorbis
$PACKAGES_ADDR/e/enca
$PACKAGES_ADDR/j/jasper
$PACKAGES_ADDR/libx/libxinerama
$PACKAGES_ADDR/x/x11proto-xinerama
$SECURITY_ADDR/c/curl
$SECURITY_ADDR/c/curl
$SECURITY_ADDR/g/gnutls26
$PACKAGES_ADDR/c/cmake
$PACKAGES_ADDR/n/nasm
$PACKAGES_ADDR/libi/libidn
$PACKAGES_ADDR/k/krb5
$PACKAGES_ADDR/o/openldap
$PACKAGES_ADDR/c/ca-certificates
$PACKAGES_ADDR/libt/libtasn1-3
$PACKAGES_ADDR/k/keyutils
$PACKAGES_ADDR/e/e2fsprogs
$PACKAGES_ADDR/c/cyrus-sasl2
$PACKAGES_ADDR/a/arts
$PACKAGES_ADDR/e/esound
$PACKAGES_ADDR/p/pulseaudio
$PACKAGES_ADDR/n/nas
$PACKAGES_ADDR/d/directfb
$PACKAGES_ADDR/a/aalib
$PACKAGES_ADDR/a/audiofile
$PACKAGES_ADDR/liba/libasyncns
$PACKAGES_ADDR/s/slang2
$PACKAGES_ADDR/g/gpm
$PACKAGES_ADDR/n/ncurses
$PACKAGES_ADDR/libc/libcap
$PACKAGES_ADDR/p/pulseaudio
$PACKAGES_ADDR/libc/libcdio
$PACKAGES_ADDR/libj/libjpeg6b
$PACKAGES_ADDR/s/sysfsutils
$PACKAGES_ADDR/t/tslib
$PACKAGES_ADDR/l/lsb
$PACKAGES_ADDR/libx/libxmu
$PACKAGES_ADDR/libx/libxt
$PACKAGES_ADDR/libo/libogg
$PACKAGES_ADDR/libs/libsamplerate
$PACKAGES_ADDR/libs/libsamplerate
$PACKAGES_ADDR/p/python2.4
$PACKAGES_ADDR/p/python2.4
$PACKAGES_ADDR/libm/libmms
$PACKAGES_ADDR/libm/libmms
)

# This is the list of package names.
DEB_FILE=(
libboost-dev_1.34.1-14_armel.deb
libmad0-dev_0.15.1b-4_armel.deb
libvorbis-dev_1.2.0.dfsg-3.1_armel.deb
libfribidi-dev_0.10.9-1_armel.deb
libmysqlclient15-dev_5.0.51a-24+lenny1_armel.deb
libpcre3-dev_7.6-2.1_armel.deb
liblzo2-dev_2.03-1_armel.deb
liblzo2-2_2.03-1_armel.deb
libsdl1.2-dev_1.2.13-2_armel.deb
libsdl1.2debian_1.2.13-2_armel.deb
libsdl1.2debian-all_1.2.13-2_armel.deb
libsdl-gfx1.2-dev_2.0.13-4_armel.deb
libsdl-gfx1.2-4_2.0.13-4_armel.deb
libsdl-image1.2-dev_1.2.6-3_armel.deb
libsdl-image1.2_1.2.6-3_armel.deb
libtiff4_3.8.2-11_armel.deb
libsdl-mixer1.2-dev_1.2.8-4_armel.deb
libsdl-mixer1.2_1.2.8-4_armel.deb
libmikmod2_3.1.11-a-6_armel.deb
libsmpeg0_0.4.5+cvs20030824-2.2_armel.deb
libvorbisfile3_1.2.0.dfsg-3.1_armel.deb
libvorbis0a_1.2.0.dfsg-3.1_armel.deb
libenca-dev_1.9-6_armel.deb
libjasper-dev_1.900.1-5.1_armel.deb
libxinerama-dev_1.0.3-2_armel.deb
x11proto-xinerama-dev_1.1.2-5_all.deb
libcurl4-gnutls-dev_7.18.2-8lenny2_armel.deb
libcurl3-gnutls_7.18.2-8lenny2_armel.deb
libgnutls26_2.4.2-6+lenny1_armel.deb
cmake_2.6.0-6_armel.deb
nasm_2.03.01-1_armel.deb
libidn11_1.8+20080606-1_armel.deb
libkrb53_1.6.dfsg.4~beta1-5lenny1_armel.deb
libldap-2.4-2_2.4.11-1_armel.deb
ca-certificates_20080809_all.deb
libtasn1-3_1.4-1_armel.deb
libkeyutils1_1.2-9_armel.deb
libcomerr2_1.41.3-1_armel.deb
libsasl2-2_2.1.22.dfsg1-23+lenny1_armel.deb
libartsc0_1.5.9-2_armel.deb
libesd0_0.2.36-3_armel.deb
libpulse0_0.9.10-3_armel.deb
libaudio2_1.9.1-5_armel.deb
libdirectfb-1.0-0_1.0.1-11_armel.deb
libaa1_1.4p5-37+b1_armel.deb
libaudiofile0_0.2.6-7_armel.deb
libasyncns0_0.3-1_armel.deb
libslang2_2.1.3-3_armel.deb
libgpm2_1.20.4-3.1_armel.deb
libncurses5_5.7+20081213-1_armel.deb
libcap1_1.10-14_armel.deb
libpulse-dev_0.9.10-3_armel.deb
libcdio-dev_0.78.2+dfsg1-3_armel.deb
libjpeg62-dev_6b-14_armel.deb
libsysfs2_2.1.0-5_armel.deb
libts-0.0-0_1.0-4_armel.deb
lsb-release_3.2-20_all.deb
libxmu6_1.0.4-1_armel.deb
libxt6_1.0.5-3_armel.deb
libogg0_1.1.3-4_armel.deb
libsamplerate0_0.1.4-1_armel.deb
libsamplerate0-dev_0.1.4-1_armel.deb
python2.4_2.4.6-1_armel.deb
python2.4-dev_2.4.6-1_armel.deb
libmms-dev_0.4-2_armel.deb
libmms0_0.4-2_armel.deb
)

echo "#### Beginning Downloads ####"
# Remove wget logfile
if [ -e wget-output.txt ]; then
	rm wget-output.txt
fi

mkdir pkgs ; cd pkgs

for (( i=0 ; i < ${#DEB_FILE[@]} ; i++ ))
do
	# If package exists, no point downloading it.
	# This means if you want to re-download,
	# you will need to remove the package manually!
	if [ -e ${DEB_FILE[$i]} ]; then
		echo -n ${DEB_FILE[$i]}
		echo " exits - Not Downloading"
	else
		echo -n ${DEB_FILE[$i]}
		echo " doesnt exist - Downloading..."
		wget ftp://${DEB_ADDR[$i]}/${DEB_FILE[$i]} >> ../wget-output.txt 2>&1
	fi
done
echo "#### Downloads Complete! Please check wget-output.txt for any errors that may have been encountered! ####"

echo "#### Extracting Packages ####"
# Only install if running from scratchbox!!! (or arm in general)
if test `uname -m` = "arm" ; then

# Remove dpkg logfile
if [ -e ../dpkg-output.txt ]; then
	rm ../dpkg-output.txt
fi

	for i in `ls *.deb`
	do
		# For each .deb package in the current directory,
		# extract the contents to / and ignore the output!
		echo "Extracting $i..."
		dpkg-deb -x $i / >> ../dpkg-output.txt 2>&1
	done
	echo "#### Extraction Complete! Please check dpkg-output.txt for any errors that may have been encountered! ####"
else
	echo "#### Extraction FAILED: Did not extract as not running in scratchbox! ####"
fi

