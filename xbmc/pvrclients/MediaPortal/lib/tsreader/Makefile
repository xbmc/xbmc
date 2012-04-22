SRCS=TSReader.cpp \
    RTSPClient.cpp \
    MemoryBuffer.cpp \
    MemoryReader.cpp \
    MemorySink.cpp
LIB=tsreader.a

include ../../../Makefile.include

INCLUDES_LIVE555=-I../live555/liveMedia/include -I../live555/BasicUsageEnvironment/include -I../live555/UsageEnvironment/include -I../live555/groupsock/include
INCLUDES+=-I. -I../../ -I../../../../filesystem
DEFINES+=-DTSREADER
#-DTSREADER

-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
