INCLUDES=-I.  -I../../.. -I../../include -I../../../utils -I.. -I../../../linux -I../../../../guilib
CFLAGS+= -fPIC
CXXFLAGS+= -fPIC
SRCS = xbmc_addon_lib++.cpp \

LIB=../../include/xbmc_addon_lib++.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
