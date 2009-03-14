CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores -I$(top_srcdir)/xbmc/cores/ffmpeg
CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/linux
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/utils -I$(top_srcdir)/guilib
CPPFLAGS	+=	-DHAS_VIDEO_PLAYBACK -D__STDC_FORMAT_MACROS

SUBDIRS	=	DVDDemuxers DVDInputStreams DVDSubtitles DVDCodecs

noinst_LIBRARIES	=	libdvdplayer.a
libdvdplayer_a_SOURCES	=	DVDAudio.cpp \
				DVDClock.cpp 
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
				DVDFileInfo.cpp

# vim: ts=8:sw=8:noexpandtab
