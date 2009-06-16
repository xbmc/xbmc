INCLUDES=-I.  -I../../.. -I../../include -I../../../utils -I.. -I../../../linux -I../../../../guilib
CFLAGS+= -fPIC
CXXFLAGS+= -fPIC
SRCS = xbmc_pvr_lib++.cpp \

LIB=../../include/xbmc_pvr_lib++.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
