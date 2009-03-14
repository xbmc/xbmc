CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/dvdplayer -I$(top_srcdir)/xbmc/cores
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/ffmpeg -I$(top_srcdir)/xbmc/
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/guilib
CPPFLAGS	+=	-DHAVE_MMX

noinst_LIBRARIES	=	libdvdaudiocodecs.a
libdvdaudiocodecs_a_SOURCES	=	DVDAudioCodecFFmpeg.cpp \
					DVDAudioCodecLiba52.cpp \
					DVDAudioCodecLibDts.cpp \
					DVDAudioCodecLibFaad.cpp \
					DVDAudioCodecLibMad.cpp \
					DVDAudioCodecLPcm.cpp \
					DVDAudioCodecPassthrough.cpp \
					DVDAudioCodecPcm.cpp

# vim: ts=8:sw=8:noexpandtab
