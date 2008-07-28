INCLUDES=-I. -I.. -I../../linux -I../../ -I ../../../guilib
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL
CFLAGS=-fPIC

SRCS=opengl_spectrum.cpp

LIB=../../../visualisations/opengl_spectrum.vis

include ../../../Makefile.include
-include $(OBJS:.o=.P)

$(LIB): $(OBJS)
	$(CXX) $(CFLAGS) -shared -g -o $(LIB) $(OBJS)

.DEFAULT_GOAL:= $(LIB)
