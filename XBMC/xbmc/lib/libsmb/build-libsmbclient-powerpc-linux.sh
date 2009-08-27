#!/bin/bash

wget http://us1.samba.org/samba/ftp/old-versions/samba-3.0.28.tar.gz
tar xfz samba-3.0.28.tar.gz

# Patch to enable compilation on ubuntu 9.04
patch -p0 < samba-3.0.28.ubtuntu-9.04-ppc.patch

cd samba-3.0.28/source
./configure --enable-static
make
mv bin/libsmbclient.a ../../libsmbclient-powerpc-linux.a

cd ../..
rm -rf samba-3.0.28 samba-3.0.28.tar.gz
