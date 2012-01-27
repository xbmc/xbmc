INCLUDES=-I. -I../.. -I../../linux

SRCS=WinBindingEGL.cpp \
     WinSystemGLES.cpp \
     
LIB=windowing_egl.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
