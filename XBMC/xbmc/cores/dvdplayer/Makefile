INCLUDES=-I. -I.. -I../../ -I../../../ -I../../linux -I../../utils -I../../../guilib
ifneq (1,1)
  INCLUDES+=-I../ffmpeg
endif


CFLAGS+=-DHAS_VIDEO_PLAYBACK -D__STDC_FORMAT_MACROS

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

LIBS=	DVDDemuxers/DVDDemuxers.a \
	DVDInputStreams/DVDInputStreams.a \
	DVDSubtitles/DVDSubtitles.a \
	DVDCodecs/DVDCodecs.a \

#DIRS=	$(dir $(LIBS))

DIRS=	DVDDemuxers \
	DVDInputStreams \
	DVDSubtitles \
	DVDCodecs \

LIB=	DVDPlayer.a

include Makefile.include








