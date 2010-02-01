#!/bin/bash 

# curl-7.19.7 configure settings cloned from MacPorts
# build against 10.4 sdk for 10.5/10.4 compatibility
if [ `arch` = "i386" ]
then
  ARCH=x86-osx
elif  [ `arch` = "ppc" ]
then
  ARCH=powerpc-osx
else
  echo "unknown `arch`"
  exit 1
fi


# build libcurl
#depends_libs: libz, libssl

wget http://curl.sourceforge.net/download/curl-7.19.7.tar.gz
tar -xzf curl-7.19.7.tar.gz
cd curl-7.19.7

sh configure MACOSX_DEPLOYMENT_TARGET=10.4 CFLAGS="-O2 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common" \
    --disable-ipv6 \
    --without-libidn \
    --without-libssh2 \
    --disable-ldap

make
mkdir -p ../include/curl
cp -f include/curl/*.h ../include/curl
gcc -bundle -flat_namespace -undefined suppress -fPIC -shared -o ../../../system/libcurl-${ARCH}.so lib/.libs/*.o
chmod +x ../../../system/libcurl-${ARCH}.so

cd ..
rm -rf curl-7.19.7 curl-7.19.7.tar.gz
