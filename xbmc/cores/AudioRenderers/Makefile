INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer -I..

CFLAGS+=

SRCS=ALSADirectSound.cpp PulseAudioDirectSound.cpp NullDirectSound.cpp AudioRendererFactory.cpp

LIB=audiorenderers.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
