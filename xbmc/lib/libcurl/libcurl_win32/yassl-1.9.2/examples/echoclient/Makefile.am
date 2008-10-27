INCLUDES = -I../../include
bin_PROGRAMS        = echoclient
echoclient_SOURCES  = echoclient.cpp
echoclient_CXXFLAGS = -Wall -Wno-unused
echoclient_LDFLAGS  = -L../../src/ -L../../taocrypt/src
echoclient_LDADD    = -lyassl -ltaocrypt
echoclient_DEPENDENCIES = ../../src/libyassl.a ../../taocrypt/src/libtaocrypt.a
EXTRA_DIST = echoclient.dsp input quit echoclient.vcproj
