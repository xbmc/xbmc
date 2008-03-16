INCLUDES=-I. -I.. -I../../../XBMC/xbmc/linux -I../../../XBMC/xbmc

SRCS=opengl_spectrum.cpp

LIB=opengl_spectrum.vis
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL

include ../../../XBMC/Makefile.include

vis:
	$(MAKE)
	$(CXX) -fpic -shared -g -o $(LIB) $(OBJS)
