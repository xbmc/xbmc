INCLUDES=-I. -I../.. -I../../linux

SRCS=WinSystemEGL.cpp \
     ../X11/XRandR.cpp
     
LIB=windowing_egl.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
