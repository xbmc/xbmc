CPPFLAGS	+=	-I$(top_srcdir)/xbmc -Iffmpeg -I$(top_srcdir)/xbmc/linux
CPPFLAGS	+=	-I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc/utils -Idvdplayer

SUBDIRS	=	AudioRenderers DllLoader ExternalPlayer VideoRenderers dvdplayer paplayer
noinst_LIBRARIES	=	libcores.a
libcores_a_SOURCES	=	DummyVideoPlayer.cpp \
				PlayerCoreFactory.cpp \
				ssrc.cpp \
				dlgcache.cpp

# vim: ts=8:sw=8:noexpandtab
