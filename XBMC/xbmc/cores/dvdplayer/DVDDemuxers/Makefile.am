CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/dvdplayer -I$(top_srcdir)/xbmc
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/ffmpeg -I$(top_srcdir)/xbmc/linux
CPPFLAGS	+=	-I$(top_srcdir)/guilib -D__STDC_CONSTANT_MACROS
CPPFLAGS	+=	-DENABLE_DVDPLAYER_HTSP

noinst_LIBRARIES	=	libdvddemuxer.a
libdvddemuxer_a_SOURCES	=	DVDDemux.cpp \
				DVDDemuxFFmpeg.cpp \
				DVDDemuxShoutcast.cpp \
				DVDDemuxUtils.cpp \
				DVDFactoryDemuxer.cpp \
				DVDDemuxVobsub.cpp \
				DVDDemuxHTSP.cpp

# vim: ts=8:sw=8:noexpandtab
