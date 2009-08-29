INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils -I../cores/ffmpeg

SRCS=PVRClient.cpp \
     PVRClientFactory.cpp \

LIB=pvrclient.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

