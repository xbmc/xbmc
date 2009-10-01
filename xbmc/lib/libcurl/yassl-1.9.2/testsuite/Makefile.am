INCLUDES = -I../include -I../taocrypt/include -I../taocrypt/mySTL
bin_PROGRAMS       = testsuite
testsuite_SOURCES  = testsuite.cpp ../taocrypt/test/test.cpp \
	../examples/client/client.cpp ../examples/server/server.cpp \
	../examples/echoclient/echoclient.cpp \
	../examples/echoserver/echoserver.cpp
testsuite_LDFLAGS  = -L../src/ -L../taocrypt/src
testsuite_CXXFLAGS = -DNO_MAIN_DRIVER -Wall -Wno-unused
testsuite_LDADD    = -lyassl -ltaocrypt
testsuite_DEPENDENCIES = ../src/libyassl.a ../taocrypt/src/libtaocrypt.a
EXTRA_DIST = testsuite.dsp test.hpp input quit make.bat testsuite.vcproj
