INCLUDES = -I../include -I../taocrypt/include -I../taocrypt/mySTL

lib_LTLIBRARIES = libyassl.la
libyassl_la_SOURCES = buffer.cpp cert_wrapper.cpp crypto_wrapper.cpp \
	handshake.cpp lock.cpp log.cpp socket_wrapper.cpp ssl.cpp \
	template_instnt.cpp timer.cpp yassl_imp.cpp yassl_error.cpp yassl_int.cpp
libyassl_la_LDFLAGS = -no-undefined -version-info 0:0:0
libyassl_la_LIBADD       = ../taocrypt/src/libtaocrypt.la
libyassl_la_DEPENDENCIES = ../taocrypt/src/libtaocrypt.la
AM_CXXFLAGS = -Wall -Wno-unused
EXTRA_DIST = ../include/*.hpp ../include/openssl/*.h make.bat yassl.cpp
