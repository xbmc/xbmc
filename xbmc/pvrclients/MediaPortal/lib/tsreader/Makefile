SRCS=TSReader.cpp \
    RTSPClient.cpp \
    MemoryBuffer.cpp \
    MemoryReader.cpp \
    MemorySink.cpp \
    Thread.cpp \
    WaitEvent.cpp
LIB=tsreader.a

include ../../../Makefile.include

INCLUDES+=-I. -I../../ -I../live555/liveMedia/include -I../live555/BasicUsageEnvironment/include -I../live555/UsageEnvironment/include -I../live555/groupsock/include
DEFINES+=-DTSREADER -DLIVE555

-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
