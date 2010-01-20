INCLUDES = -I../include -I../mySTL

lib_LTLIBRARIES = libtaocrypt.la
libtaocrypt_la_SOURCES  = aes.cpp aestables.cpp algebra.cpp arc4.cpp asn.cpp \
	bftables.cpp blowfish.cpp coding.cpp dh.cpp des.cpp dsa.cpp file.cpp \
    hash.cpp integer.cpp md2.cpp md4.cpp md5.cpp misc.cpp random.cpp \
    ripemd.cpp rsa.cpp sha.cpp template_instnt.cpp tftables.cpp twofish.cpp \
    crypto.cpp rabbit.cpp hc128.cpp
libtaocrypt_la_LDFLAGS = -no-undefined -version-info 0:0:0
AM_CXXFLAGS = -Wall -Wno-return-type -Wno-unused
EXTRA_DIST = ../include/*.hpp ../test/memory.cpp make.bat
