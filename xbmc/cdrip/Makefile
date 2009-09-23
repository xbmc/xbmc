INCLUDES=-I. -I../ -I../linux -I../../guilib

SRCS=CDDARipper.cpp \
     CDDAReader.cpp \
     Encoder.cpp \
     EncoderLame.cpp \
     EncoderVorbis.cpp \
     EncoderWav.cpp \

LIB=cdrip.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
