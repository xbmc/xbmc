#!/bin/bash 

# curl-7.19.0 configure settings cloned from MacPorts
# build against 10.4 sdk for 10.5/10.4 compatibility

# Get ARCH from Makefile.include
file=../../../Makefile.include
if [ -f $file ]
then
  ARCH=$(grep "^ARCH=" $file | awk -F"=" '{print $2}')
else
  echo "$file not found... You must run configure first!"
  exit 1
fi


# build libcurl
#depends_libs: libz

#wget http://curl.sourceforge.net/download/curl-7.19.0.tar.gz
#tar -xzf curl-7.19.0.tar.gz
cd curl-7.19.0

sh configure MACOSX_DEPLOYMENT_TARGET=10.4 CFLAGS="-O2 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common" \
    --disable-ipv6 \
    --without-libidn \
    --without-libssh2 \
    --without-ssl \
    --disable-ldap

make
gcc -bundle -flat_namespace -undefined suppress -fPIC -shared -o ../../../../system/libcurl-${ARCH}.so lib/.libs/*.o
chmod +x ../../../../system/libcurl-${ARCH}.so

cd ..
#rm -rf curl-7.19.0 curl-7.19.0.tar.gz
