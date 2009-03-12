INCLUDES=-I. -I../../ -I../../linux -I../../../guilib -I../../utils -I../dvdplayer -I..

SRCS = \
	MasterAudioCore.cpp \
	AudioManager.cpp \
	AudioManagerClient.cpp \
	PCMAudioClient.cpp \
	PassthroughAudioClient.cpp \
	DirectSoundAdapter.cpp \
	Util/SimpleBuffer.cpp	\
	Util/WaveFileRenderer.cpp \
	Filters/AC3Encoder.cpp

LIB=masteraudio.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
