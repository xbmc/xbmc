INCLUDES = -I../../include
bin_PROGRAMS    = client
client_SOURCES  = client.cpp
client_CXXFLAGS = -Wall -Wno-unused
client_LDFLAGS  = -L../../src/ -L../../taocrypt/src
client_LDADD    = -lyassl -ltaocrypt
client_DEPENDENCIES = ../../src/libyassl.a ../../taocrypt/src/libtaocrypt.a
EXTRA_DIST = client.dsp client.vcproj
