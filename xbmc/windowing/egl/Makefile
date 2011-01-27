INCLUDES=-I. -I../.. -I../../linux

SRCS=WinSystemGLES.cpp \
     
LIB=windowing_gles.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
