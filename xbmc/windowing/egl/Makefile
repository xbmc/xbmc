INCLUDES=-I.

SRCS = WinSystemEGL.cpp
SRCS+= EGLNativeTypeAmlogic.cpp
SRCS+= EGLNativeTypeAndroid.cpp
SRCS+= EGLWrapper.cpp

LIB = windowing_egl.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
