SRCS = WsgiErrorStream.cpp \
       WsgiInputStream.cpp \
       WsgiResponse.cpp \
       WsgiResponseBody.cpp \

LIB=legacy-wsgi.a

CXXFLAGS+= -DSWIGRUNTIME_DEBUG -DSTATIC_LINKED

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

