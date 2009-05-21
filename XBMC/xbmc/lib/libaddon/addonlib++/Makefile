INCLUDES=-I.  -I../../../../addons -I../../../utils -I..
CFLAGS+= -fPIC
CXXFLAGS+= -fPIC
SRCS = xbmc_addon_lib++.cpp \

LIB=../../../../addons/xbmc_addon_lib++.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
