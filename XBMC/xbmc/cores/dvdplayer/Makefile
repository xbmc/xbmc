INCLUDES=-I. -I.. -I../ffmpeg -I../../ -I../../linux -I../../utils -I../../../guilib

CXXFLAGS+=-DHAS_VIDEO_PLAYBACK -D__STDC_FORMAT_MACROS

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
	DVDStreamInfo.cpp \
	DVDFileInfo.cpp \

LIB=	DVDPlayer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

