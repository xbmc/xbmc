INCLUDES=-I. -I.. -I../../../ -I../../../linux -I../../../../guilib -I../../../lib/libRTMP
CXXFLAGS += -D__STDC_FORMAT_MACROS \
          -DENABLE_DVDINPUTSTREAM_STACK \
          -DENABLE_DVDPLAYER_HTSP \

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
	DVDInputStreamHTSP.cpp \

LIB=	DVDInputStreams.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

