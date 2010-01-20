INCLUDES = -I../../include
bin_PROGRAMS        = echoclient
echoclient_SOURCES  = echoclient.cpp
echoclient_CXXFLAGS = -Wall -Wno-unused
echoclient_LDFLAGS  = -L../../src/ -L../../taocrypt/src
echoclient_LDADD    = ../../src/libyassl.la ../../taocrypt/src/libtaocrypt.la
echoclient_DEPENDENCIES = ../../src/libyassl.la \
                          ../../taocrypt/src/libtaocrypt.la
EXTRA_DIST = echoclient.dsp input quit echoclient.vcproj
