INCLUDES=-I. -I.. -I../../ -I../ffmpeg -I../../linux -I../../utils -I../../../guilib -I../../lib

CXXFLAGS+=-D__STDC_FORMAT_MACROS

SRCS=	DVDAudio.cpp \
	DVDClock.cpp \
	DVDDemuxSPU.cpp \
	DVDMessage.cpp \
	DVDMessageQueue.cpp \
	DVDMessageTracker.cpp \
	DVDOverlayContainer.cpp \
	DVDOverlayRenderer.cpp \
	DVDPerformanceCounter.cpp \
	DVDPlayerAudio.cpp \
	DVDPlayer.cpp \
	DVDPlayerSubtitle.cpp \
	DVDPlayerVideo.cpp \
	DVDPlayerTeletext.cpp \
	DVDStreamInfo.cpp \
	DVDFileInfo.cpp \
	DVDPlayerAudioResampler.cpp \
	DVDTSCorrection.cpp \
	Edl.cpp

LIB=	DVDPlayer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

