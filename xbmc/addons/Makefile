INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils -I../cores/ffmpeg -Iinclude

SRCS=AddonHelpers_Addon.cpp \
     AddonHelpers_PVR.cpp \
     AddonHelpers_local.cpp \

LIB=AddonHelpers.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

