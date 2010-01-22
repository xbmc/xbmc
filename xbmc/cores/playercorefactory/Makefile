INCLUDES=-I../../.. -I. -I../ -I../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer -I../paplayer -I../dvdplayer/Codecs/ffmpeg

SRCS=PlayerCoreFactory.cpp \
     PlayerSelectionRule.cpp

LIB=playercorefactory.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

