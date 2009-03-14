CPPFLAGS	+=	-I$(top_srcdir)/xbmc/cores -I$(top_srcdir)/xbmc/
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/guilib
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/utils

SUBDIRS	=	VideoShaders

noinst_LIBRARIES	=	libvideorenderer.a
libvideorenderer_a_SOURCES	=	LinuxRenderer.cpp \
					RenderManager.cpp \
					LinuxRendererGL.cpp

# vim: ts=8:sw=8:noexpandtab
