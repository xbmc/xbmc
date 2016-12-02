SRCS=PixelConverter.cpp \
     RetroPlayer.cpp \
     RetroPlayerAudio.cpp \
     RetroPlayerVideo.cpp

ifeq (@CORE_SYSTEM_NAME@,rbpi)
  SRCS += PixelConverterRBP.cpp
endif

LIB=retroplayer.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
