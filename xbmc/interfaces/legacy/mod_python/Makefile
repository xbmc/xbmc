SRCS = Cookie.cpp \
       HttpRequest.cpp \

LIB=legacy-mod_python.a

CXXFLAGS+= -DSWIGRUNTIME_DEBUG -DSTATIC_LINKED

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
