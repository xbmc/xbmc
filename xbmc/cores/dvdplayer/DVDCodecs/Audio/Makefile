INCLUDES=-I. -I../../ -I../../.. -I../../../../ -I../../../../../ -I../../../../linux -I../../../../../guilib
ifneq (1,1)
  INCLUDES+=-I../../../ffmpeg
endif

CFLAGS+=-DHAVE_MMX

SRCS=	DVDAudioCodecFFmpeg.cpp \
	DVDAudioCodecLiba52.cpp \
	DVDAudioCodecLibDts.cpp \
	DVDAudioCodecLibFaad.cpp \
	DVDAudioCodecLibMad.cpp \
	DVDAudioCodecLPcm.cpp \
	DVDAudioCodecPassthrough.cpp \
	DVDAudioCodecPcm.cpp \

LIB=Audio.a

include ../../Makefile.include

