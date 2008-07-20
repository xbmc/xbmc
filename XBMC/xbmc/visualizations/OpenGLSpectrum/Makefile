INCLUDES=-I. -I.. -I../../linux -I../../ -I ../../../guilib

SRCS=opengl_spectrum.cpp

LIB=opengl_spectrum.vis
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL

include ../../../Makefile.include

lib: $(OBJS) vis

vis:
	$(CXX) -fpic -shared -g -o $(LIB) $(OBJS) && cp $(LIB) ../../../visualisations
