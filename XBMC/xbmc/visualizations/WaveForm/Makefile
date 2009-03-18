INCLUDES=-I. -I../../linux -I../../ -I ../../../guilib
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL
CFLAGS=-fPIC
SRCS=Main.cpp

LIB=../../../visualisations/Waveform.vis

include ../../../Makefile.include
-include $(OBJS:.o=.P)

$(LIB): $(OBJS)
ifeq ($(findstring osx,$(ARCH)), osx)
	@export MACOSX_DEPLOYMENT_TARGET=10.4
	$(CC) -bundle -shared -flat_namespace -undefined suppress -o $@ $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@

else
	$(CXX) $(CFLAGS) -shared -g -o $(LIB) $(OBJS)

endif

.DEFAULT_GOAL:= $(LIB)
