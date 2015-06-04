SRCS=WinSystemX11.cpp \
     WinSystemX11GLContext.cpp \
     GLContextEGL.cpp \
     GLContextGLX.cpp \
     XRandR.cpp \
     
LIB=windowing_X11.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
