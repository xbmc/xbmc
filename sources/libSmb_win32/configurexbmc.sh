#!/bin/sh

###################################################################################################
echo "[ Running configure script for XBMC ]"
###################################################################################################
chmod +x configure && ./configure \
--build=xbox
--disable-libmsrpc \
--disable-pie \
--disable-iprint \
--without-ldap \
--without-cifsmount \
--without-libsmbsharemodes \
--without-sendfile-support \
$@