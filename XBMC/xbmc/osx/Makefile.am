CPPFLAGS	+=	-I$(top_scrdir)/xbmc -I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc/linux
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/utils -I$(top_srcdir)/xbmc/FileSystem 
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libportaudio

noinst_LIBRARIES	=	libosx.a
libosx_a_SOURCES	=	XBMCMain.m \
				XBMCHelper.m \
				CocoaUtils.m \
				OSXGNUReplacements.c \
				getdelim.c \
				CPortAudio.cpp

# vim: ts=8:sw=8:noexpandtab
