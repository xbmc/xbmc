INCLUDES=-I. -I.. -I../../ -I../../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer/Codecs/ffmpeg -I../dvdplayer

SRCS=LinuxRenderer.cpp \
     RenderManager.cpp \
     LinuxRendererGL.cpp \
     OverlayRenderer.cpp \
     OverlayRendererGL.cpp \
     OverlayRendererUtil.cpp \
     BaseRenderer.cpp

LIB=VideoRenderer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
