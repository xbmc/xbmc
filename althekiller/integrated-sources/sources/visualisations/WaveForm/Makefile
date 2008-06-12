INCLUDES=-I. -I.. -I../../../XBMC/xbmc/linux -I../../../XBMC/xbmc

SRCS=Main.cpp

LIB=Waveform.vis
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL

include ../../../XBMC/Makefile.include

vis:
	$(MAKE)
	$(CXX) -fpic -shared -o $(LIB) $(OBJS)
