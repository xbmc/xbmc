CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores/dvdplayer -I$(top_srcdir)/xbmc/cores
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libRTMP -D__STDC_FORMAT_MACROS
CPPFLAGS	+=	-DENABLE_DVDINPUTSTREAM_STACK -DENABLE_DVDPLAYER_HTSP

noinst_LIBRARIES	=	libdvdinputstream.a
libdvdinputstream_a_SOURCES	=	DVDFactoryInputStream.cpp \
					DVDInputStream.cpp \
					DVDInputStreamFile.cpp \
					DVDInputStreamHttp.cpp \
					DVDInputStreamMemory.cpp \
					DVDInputStreamNavigator.cpp \
					DVDStateSerializer.cpp \
					DVDInputStreamFFmpeg.cpp \
					DVDInputStreamTV.cpp \
					DVDInputStreamRTMP.cpp \
					DVDInputStreamStack.cpp \
					DVDInputStreamHTSP.cpp

# vim: ts=8:sw=8:noexpandtab
