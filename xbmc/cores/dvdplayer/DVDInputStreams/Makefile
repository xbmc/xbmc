INCLUDES=-I. -I.. -I../../../ -I../../../linux -I../../../../guilib -I../../../lib/libRTMP
CFLAGS += -D__STDC_FORMAT_MACROS \
          -DENABLE_DVDINPUTSTREAM_STACK \

SRCS=	DVDFactoryInputStream.cpp \
	DVDInputStream.cpp \
	DVDInputStreamFile.cpp \
	DVDInputStreamHttp.cpp \
	DVDInputStreamMemory.cpp \
	DVDInputStreamNavigator.cpp \
	DVDStateSerializer.cpp \
	DVDInputStreamFFmpeg.cpp \
	DVDInputStreamTV.cpp \
	DVDInputStreamRTMP.cpp \
	DVDInputStreamStack.cpp \

LIB=	DVDInputStreams.a

include ../Makefile.include

