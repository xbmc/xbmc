INCLUDES=-I. -I../ -I../../../ -I../.. -I../../ffmpeg -I../../../linux -I../../../../guilib

SRCS=DVDCodecUtils.cpp DVDFactoryCodec.cpp

LIB=dvdcodecs.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
