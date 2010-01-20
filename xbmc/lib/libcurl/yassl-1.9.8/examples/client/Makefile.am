INCLUDES = -I../../include
bin_PROGRAMS    = client
client_SOURCES  = client.cpp
client_CXXFLAGS = -Wall -Wno-unused
client_LDFLAGS  = -L../../src/ -L../../taocrypt/src
client_LDADD        = ../../src/libyassl.la ../../taocrypt/src/libtaocrypt.la
client_DEPENDENCIES = ../../src/libyassl.la ../../taocrypt/src/libtaocrypt.la
EXTRA_DIST = client.dsp client.vcproj
