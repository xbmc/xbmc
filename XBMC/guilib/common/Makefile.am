CPPFLAGS	+=	-I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc 
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/linux -I$(top_srcdir)/xbmc/utils
noinst_LIBRARIES	=	libgui_common.a
libgui_common_a_SOURCES	=	Keyboard.cpp \
				Mouse.cpp \
				SDLJoystick.cpp \
				SDLKeyboard.cpp \
				SDLMouse.cpp
if XBMC_PLAT_LINUX
libgui_common_a_SOURCES	+=	LIRC.cpp
endif

# vim: ts=8:sw=8:noexpandtab
