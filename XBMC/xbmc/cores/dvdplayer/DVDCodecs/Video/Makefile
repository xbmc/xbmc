INCLUDES=-I. -I../../ -I../../../ffmpeg -I../../../../ -I../../../../linux -I../../../../../guilib

SRCS=DVDVideoCodecFFmpeg.cpp DVDVideoCodecLibMpeg2.cpp DVDVideoPPFFmpeg.cpp

LIB=dvdcodecs_video.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
