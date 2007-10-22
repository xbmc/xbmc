INCLUDES=-I. -I.. -I../../xbmc/linux -I../../xbmc

SRCS=Main.cpp

LIB=ProjectM.vis
SDL_DEFINES=-DHAS_SDL_OPENGL -DHAS_SDL

include ../../Makefile.include

vis:
	$(MAKE)
	$(CXX) ./FTGL/unix/src/*.o ./libprojectM/src/*.o -fPIC -shared -g -o $(LIB) $(OBJS)
#./FTGL/unix/src/*.o ./libprojectM/src/*.o
