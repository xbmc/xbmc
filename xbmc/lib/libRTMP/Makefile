INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils

SRCS=rtmp.cpp \
     rtmppacket.cpp \
     AMFObject.cpp \

LIB=librtmp.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
