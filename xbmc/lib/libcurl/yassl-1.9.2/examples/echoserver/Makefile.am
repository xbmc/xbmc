INCLUDES = -I../../include
bin_PROGRAMS        = echoserver
echoserver_SOURCES  = echoserver.cpp
echoserver_CXXFLAGS = -Wall -Wno-unused
echoserver_LDFLAGS  = -L../../src/ -L../../taocrypt/src
echoserver_LDADD    = -lyassl -ltaocrypt
echoserver_DEPENDENCIES = ../../src/libyassl.a ../../taocrypt/src/libtaocrypt.a
EXTRA_DIST = echoserver.dsp echoserver.vcproj
