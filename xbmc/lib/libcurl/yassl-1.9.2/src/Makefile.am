INCLUDES = -I../include -I../taocrypt/include -I../taocrypt/mySTL

lib_LIBRARIES = libyassl.a
libyassl_a_SOURCES = buffer.cpp cert_wrapper.cpp crypto_wrapper.cpp \
	handshake.cpp lock.cpp log.cpp socket_wrapper.cpp ssl.cpp \
	template_instnt.cpp timer.cpp yassl_imp.cpp yassl_error.cpp yassl_int.cpp
AM_CXXFLAGS = -Wall -Wno-unused
EXTRA_DIST = ../include/*.hpp ../include/openssl/*.h make.bat yassl.cpp
