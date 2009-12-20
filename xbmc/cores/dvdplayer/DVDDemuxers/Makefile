INCLUDES=-I. -I.. -I../.. -I../../../ -I../Codecs/ffmpeg -I../../../utils -I../../../linux -I../../../../guilib
CXXFLAGS+=-D__STDC_CONSTANT_MACROS \

SRCS=	DVDDemux.cpp \
	DVDDemuxFFmpeg.cpp \
	DVDDemuxShoutcast.cpp \
	DVDDemuxUtils.cpp \
	DVDFactoryDemuxer.cpp \
	DVDDemuxVobsub.cpp \
	DVDDemuxHTSP.cpp \

LIB=	DVDDemuxers.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

