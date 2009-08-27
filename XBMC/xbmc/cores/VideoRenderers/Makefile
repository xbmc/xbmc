INCLUDES=-I. -I.. -I../../ -I../../../ -I../../linux -I../../../guilib -I../../utils

SRCS=LinuxRenderer.cpp \
     RenderManager.cpp \
     LinuxRendererGL.cpp \

LIB=VideoRenderer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
