CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/dvdplayer -I$(top_srcdir)/xbmc/cores
CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/cores/ffmpeg
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/guilib

SUBDIRS	=   Audio Video Overlay

noinst_LIBRARIES	=	libdvdcodec.a
libdvdcodec_a_SOURCES	=	DVDCodecUtils.cpp \
				DVDFactoryCodec.cpp

# vim: ts=8:sw=8:noexpandtab
