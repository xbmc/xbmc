INCLUDES=-I.  -I../../.. -I../../../addons/IndependentHeaders -I../../../utils -I..
CFLAGS+= -fPIC
CXXFLAGS+= -fPIC

SRCS = xbmc_addon_lib.c \

LIB=../../../addons/IndependentHeaders/xbmc_addon_lib.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
