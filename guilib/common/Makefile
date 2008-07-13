INCLUDES=-I. -I../ -I../../xbmc -I../../xbmc/linux -I../../xbmc/utils

SRCS=SDLMouse.cpp Mouse.cpp SDLKeyboard.cpp LIRC.cpp SDLJoystick.cpp Keyboard.cpp

LIB=gui_common.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
