INCLUDES = -I../include -I../taocrypt/include -I../taocrypt/mySTL
bin_PROGRAMS       = testsuite
testsuite_SOURCES  = testsuite.cpp ../taocrypt/test/test.cpp \
	../examples/client/client.cpp ../examples/server/server.cpp \
	../examples/echoclient/echoclient.cpp \
	../examples/echoserver/echoserver.cpp
testsuite_LDFLAGS  = -L../src/ -L../taocrypt/src
testsuite_CXXFLAGS = -DNO_MAIN_DRIVER -Wall -Wno-unused
testsuite_LDADD        = ../src/libyassl.la ../taocrypt/src/libtaocrypt.la
testsuite_DEPENDENCIES = ../src/libyassl.la ../taocrypt/src/libtaocrypt.la
EXTRA_DIST = testsuite.dsp test.hpp input quit make.bat testsuite.vcproj
