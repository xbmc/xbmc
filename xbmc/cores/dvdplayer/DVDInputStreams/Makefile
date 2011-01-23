INCLUDES=-I. -I.. -I../.. -I../../../ -I../../../linux -I../../../../guilib -I../../../lib/libRTMP -I../../../lib
CXXFLAGS += -D__STDC_FORMAT_MACROS \
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
	DVDInputStreamHTSP.cpp \
	DVDInputStreamMMS.cpp \
	DVDInputStreamBluray.cpp \

LIB=	DVDInputStreams.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

