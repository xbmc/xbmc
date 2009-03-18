INCLUDES=-I. -I.. -I../../../ -I../../ffmpeg -I../../../linux -I../../../../guilib 
CFLAGS+=-D__STDC_CONSTANT_MACROS \
        -DENABLE_DVDPLAYER_HTSP \

SRCS=	DVDDemux.cpp \
	DVDDemuxFFmpeg.cpp \
	DVDDemuxShoutcast.cpp \
	DVDDemuxUtils.cpp \
	DVDFactoryDemuxer.cpp \
	DVDDemuxVobsub.cpp \
	DVDDemuxHTSP.cpp \

LIB=	DVDDemuxers.a

include ../Makefile.include

