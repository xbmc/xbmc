INCLUDES=-I. -I.. -I../../../XBMC/xbmc/linux -I../../../XBMC/xbmc -I../../../XBMC/guilib

SRCS=Main.cpp

LIB=ProjectM.vis
SDL_DEFINES=-DHAS_SDL_OPENGL -DHAS_SDL
CFLAGS=-O3

include ../../../XBMC/Makefile.include

vis:
	$(MAKE)
	$(CXX) ./libprojectM/CMakeFiles/projectM.dir/*.o -fPIC -shared -O3 -o $(LIB) $(OBJS)
