CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/linux
CPPFLAGS	+=	-I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc/utils
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/dvdplayer -I$(top_srcdir)/xbmc/cores
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libshout

noinst_LIBRARIES	=	libpaplayer.a
libpaplayer_a_SOURCES	=	AACcodec.cpp \
				AC3CDDACodec.cpp \
				AC3Codec.cpp \
				ADPCMCodec.cpp \
				AdplugCodec.cpp \
				AIFFcodec.cpp \
				APEcodec.cpp \
				AudioDecoder.cpp \
				CDDAcodec.cpp \
				CodecFactory.cpp \
				VGMCodec.cpp \
				FLACcodec.cpp \
				GYMCodec.cpp \
				ModuleCodec.cpp \
				MP3codec.cpp \
				MPCcodec.cpp \
				NSFCodec.cpp \
				OGGcodec.cpp \
				ReplayGain.cpp \
				SHNcodec.cpp \
				SIDCodec.cpp \
				SPCCodec.cpp \
				TimidityCodec.cpp \
				WAVcodec.cpp \
				WAVPackcodec.cpp \
				WMACodec.cpp \
				YMCodec.cpp \
				DVDPlayerCodec.cpp \
				ASAPCodec.cpp \
				DTSCodec.cpp \
				DTSCDDACodec.cpp

if XBMC_PLAT_OSX
libpaplayer_a_SOURCES	+=	paplayer_osx.cpp
else
CPPFLAGS		+=	-DHAS_ALSA # TODO: use autoheader instead.
libpaplayer_a_SOURCES	+=	paplayer_linux.cpp
endif

# vim: ts=8:sw=8:noexpandtab
