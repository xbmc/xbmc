INCLUDES=-I. -I.. -I../../ -I../../linux -I../../../guilib

ifeq ($(SDL_DEFINES),-DHAS_SDL_2D)
SRCS=LinuxRenderer.cpp RenderManager.cpp
else
SRCS=LinuxRendererGL.cpp RenderManager.cpp
endif


LIB=VideoRenderer.a


include ../../../Makefile.include
-include Makefile.depend

clean:
	$(RM) *.o $(LIB)
