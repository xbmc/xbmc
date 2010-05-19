#!/bin/sh
#
# Download and install gcc-4.5 for arm platform. This is a native install
# and not a cross-compiler install.
#

# The URL sources
URLS="http://ftp.debian.org/debian/pool/main/b/binutils/binutils_2.20.51.20100418-1_armel.deb \
http://ftp.debian.org/debian/pool/main/c/cloog-ppl/libcloog-ppl0_0.15.9-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/cpp-4.5_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/g++-4.5_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/gcc-4.5_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/gcc-4.5-base_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/libgcc1_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/libgomp1_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/libstdc++6_4.5-20100321-1_armel.deb \ 
http://ftp.debian.org/debian/pool/main/g/gcc-4.5/libstdc++6-4.5-dev_4.5-20100321-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gmpc/gmpc_0.20.0-1_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gmp/libgmp10_5.0.1+dfsg-2_armel.deb \
http://ftp.debian.org/debian/pool/main/g/gmp/libgmpxx4ldbl_5.0.1+dfsg-2_armel.deb \ 
http://ftp.debian.org/debian/pool/main/g/gmp/libmp3_5.0.1+dfsg-2_armel.deb \
http://ftp.debian.org/debian/pool/main/libe/libelf/libelfg0_0.8.13-1_armel.deb \
http://ftp.debian.org/debian/pool/main/m/mpclib/libmpc2_0.8.1-1_armel.deb \
http://ftp.debian.org/debian/pool/main/p/ppl/libppl7_0.10.2-6_armel.deb \
http://ftp.debian.org/debian/pool/main/p/ppl/libppl-c2_0.10.2-6_armel.deb"

# Download them using wget
mkdir -p gcc-4.5-debs
for u in $URLS; do wget --directory-prefix=./gcc-4.5-debs $u; done

# Install gcc-4.5
dpkg -i ./gcc-4.5-debs/*.deb

