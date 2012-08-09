INCLUDES=-I.

SRCS = WinSystemGLES.cpp
SRCS+= WinEGLPlatformGeneric.cpp
SRCS+= WinEGLPlatformAndroid.cpp
     
LIB = windowing_egl.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
