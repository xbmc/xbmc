INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer -I..

SRCS = \
	MasterAudioCore.cpp \
	AudioManager.cpp \
	AudioManagerClient.cpp \
	PCMAudioClient.cpp \
	DirectSoundAdapter.cpp
	SimpleBuffer.cpp

LIB=masteraudio.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
