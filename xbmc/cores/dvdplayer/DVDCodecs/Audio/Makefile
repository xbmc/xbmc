INCLUDES=-I. -I../../ -I../../.. -I../../../ffmpeg -I../../../../ -I../../../../linux -I../../../../../guilib

SRCS=DVDAudioCodecFFmpeg.cpp DVDAudioCodecLiba52.cpp DVDAudioCodecLibDts.cpp DVDAudioCodecLibFaad.cpp DVDAudioCodecLibMad.cpp DVDAudioCodecLPcm.cpp DVDAudioCodecPassthrough.cpp DVDAudioCodecPcm.cpp

LIB=dvdcodecs_audio.a

include ../../../../../Makefile.include
-include Makefile.depend

CFLAGS+=-DHAVE_MMX
