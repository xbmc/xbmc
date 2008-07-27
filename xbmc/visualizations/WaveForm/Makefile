INCLUDES=-I. -I../../linux -I../../ -I ../../../guilib
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL

SRCS=Main.cpp

LIB=../../../visualisations/Waveform.vis

include ../../../Makefile.include
-include $(OBJS:.o=.P)

$(LIB): $(OBJS)
	$(CXX) -fpic -shared -g -o $(LIB) $(OBJS)

.DEFAULT_GOAL:= $(LIB)
