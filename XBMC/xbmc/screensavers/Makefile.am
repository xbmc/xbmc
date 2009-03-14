CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/linux 
CPPFLAGS	+=	-I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc/utils
CFLAGS		+=	-fPIC #FIXME?

if XBMC_PLAT_OSX
CPPFLAGS	+=	-I/opt/local/include
CFLAGS		+=	-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif

noinst_LIBRARIES	=	libscreensavers.a
libscreensavers_a_SOURCES	=	ScreenSaver.cpp \
					ScreenSaverFactory.cpp

# vim: ts=8:sw=8:noexpandtab
