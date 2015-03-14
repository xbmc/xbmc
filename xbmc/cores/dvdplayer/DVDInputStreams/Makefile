CXXFLAGS += -D__STDC_FORMAT_MACROS \
          -DENABLE_DVDINPUTSTREAM_STACK \

SRCS=	DVDFactoryInputStream.cpp \
	DVDInputStream.cpp \
	DVDInputStreamBluray.cpp \
	DVDInputStreamFFmpeg.cpp \
	DVDInputStreamFile.cpp \
	DVDInputStreamHttp.cpp \
	DVDInputStreamMemory.cpp \
	DVDInputStreamNavigator.cpp \
	DVDInputStreamRTMP.cpp \
	DVDInputStreamPVRManager.cpp \
	DVDInputStreamStack.cpp \
	DVDInputStreamTV.cpp \
	DVDStateSerializer.cpp \

LIB=	DVDInputStreams.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

