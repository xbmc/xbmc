INCLUDES = -I../../include
bin_PROGRAMS    = server
server_SOURCES  = server.cpp
server_CXXFLAGS = -Wall -Wno-unused
server_LDFLAGS  = -L../../src/ -L../../taocrypt/src
server_LDADD        = ../../src/libyassl.la ../../taocrypt/src/libtaocrypt.la 
server_DEPENDENCIES = ../../src/libyassl.la ../../taocrypt/src/libtaocrypt.la
EXTRA_DIST = server.dsp server.vcproj
