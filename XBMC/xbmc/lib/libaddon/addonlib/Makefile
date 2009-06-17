INCLUDES=-I.  -I../../.. -I../../../addons/include -I../../../utils -I..
CFLAGS+= -fPIC
CXXFLAGS+= -fPIC

SRCS = xbmc_addon_lib.c \
			 settings.c \
			 setting.c \

LIB=../../../addons/include/xbmc_addon_lib.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
