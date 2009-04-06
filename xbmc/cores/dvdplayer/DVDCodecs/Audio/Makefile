INCLUDES=-I. -I../../ -I../../.. -I../../../ffmpeg -I../../../../ -I../../../../linux -I../../../../../guilib
CXXFLAGS+=-DHAVE_MMX

SRCS=	DVDAudioCodecFFmpeg.cpp \
	DVDAudioCodecLiba52.cpp \
	DVDAudioCodecLibDts.cpp \
	DVDAudioCodecLibFaad.cpp \
	DVDAudioCodecLibMad.cpp \
	DVDAudioCodecLPcm.cpp \
	DVDAudioCodecPassthrough.cpp \
	DVDAudioCodecPcm.cpp \

LIB=Audio.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

