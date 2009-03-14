CPPFLAGS	+=	-I$(top_srcdir)/xbmc  -I$(top_srcdir)/xbmc/linux 
CPPFLAGS	+=	-I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc/utils
CPPFLAGS	+=	-I$(top_srcdir)/dvdplayer -I$(top_srcdir)/xbmc/cores

noinst_LIBRARIES	=	libaudiorenderer.a
libaudiorenderer_a_SOURCES	=	NullDirectSound.cpp \
					AudioRendererFactory.cpp

if XBMC_PLAT_OSX
libaudiorenderer_a_SOURCES	+=	PortaudioDirectSound.cpp
endif 
if XBMC_PLAT_LINUX
libaudiorenderer_a_SOURCES	+=	ALSADirectSound.cpp \
					PulseAudioDirectSound.cpp
endif

# vim: ts=8:sw=8:noexpandtab
