INCLUDES = -I../../include
bin_PROGRAMS        = echoserver
echoserver_SOURCES  = echoserver.cpp
echoserver_CXXFLAGS = -Wall -Wno-unused
echoserver_LDFLAGS  = -L../../src/ -L../../taocrypt/src
echoserver_LDADD    = ../../src/libyassl.la ../../taocrypt/src/libtaocrypt.la
echoserver_DEPENDENCIES = ../../src/libyassl.la \
                          ../../taocrypt/src/libtaocrypt.la
EXTRA_DIST = echoserver.dsp echoserver.vcproj
