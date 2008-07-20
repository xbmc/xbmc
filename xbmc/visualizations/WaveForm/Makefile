INCLUDES=-I. -I../../linux -I../../ -I ../../../guilib

SRCS=Main.cpp

LIB=Waveform.vis
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL

include ../../../Makefile.include

lib: $(OBJS) vis

vis:
	$(CXX) -fpic -shared -o $(LIB) $(OBJS) && cp $(LIB) ../../../visualisations/
