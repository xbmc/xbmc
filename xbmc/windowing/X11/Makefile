SRCS=WinSystemX11.cpp \
     WinSystemX11GL.cpp \
     WinSystemX11GLES.cpp \
     XRandR.cpp \
     
LIB=windowing_X11.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
