INCLUDES=-I.  -I../../../../addons -I../../../utils -I..
CFLAGS+= -fPIC
CXXFLAGS+= -fPIC

SRCS = xbmc_pvr_lib.c \

LIB=../../../../addons/xbmc_pvr_lib.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
