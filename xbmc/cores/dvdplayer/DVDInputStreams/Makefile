<<<<<<< HEAD
=======
INCLUDES=-I. -I.. -I../.. -I../../../ -I../../../linux -I../../../../guilib -I../../../lib/libRTMP -I../../../lib
>>>>>>> FETCH_HEAD
CXXFLAGS += -D__STDC_FORMAT_MACROS \
          -DENABLE_DVDINPUTSTREAM_STACK \

SRCS=	DVDFactoryInputStream.cpp \
	DVDInputStream.cpp \
	DVDInputStreamBluray.cpp \
	DVDInputStreamFFmpeg.cpp \
	DVDInputStreamFile.cpp \
	DVDInputStreamHTSP.cpp \
	DVDInputStreamHttp.cpp \
	DVDInputStreamMemory.cpp \
	DVDInputStreamNavigator.cpp \
	DVDInputStreamRTMP.cpp \
	DVDInputStreamPVRManager.cpp \
	DVDInputStreamStack.cpp \
<<<<<<< HEAD
	DVDInputStreamTV.cpp \
	DVDStateSerializer.cpp \
=======
	DVDInputStreamHTSP.cpp \
	DVDInputStreamMMS.cpp \
	DVDInputStreamBluray.cpp \
>>>>>>> FETCH_HEAD

LIB=	DVDInputStreams.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

