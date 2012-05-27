SRCS=TSReader.cpp \
    RTSPClient.cpp \
    MemoryBuffer.cpp \
    MemoryReader.cpp \
    MemorySink.cpp \
    FileReader.cpp \
    MultiFileReader.cpp \
    posix/FileSMB.cpp \
    $(wildcard ../live555/BasicUsageEnvironment/*.cpp) \
    $(wildcard ../live555/UsageEnvironment/*.cpp) \
    $(wildcard ../live555/groupsock/*.cpp) \
    $(wildcard ../live555/groupsock/*.c) \
    $(wildcard ../live555/liveMedia/*.cpp) \
    $(wildcard ../live555/liveMedia/*.c)
LIB=tsreader.a

include ../../../Makefile.include

INCLUDES_LIVE555=-I../live555/liveMedia/include -I../live555/BasicUsageEnvironment/include -I../live555/UsageEnvironment/include -I../live555/groupsock/include
DEFINES_LIVE555=-DLIVE555 -DSOCKLEN_T=socklen_t -DBSD=1

INCLUDES+=-I. -I../../ -I../../../../filesystem $(INCLUDES_LIVE555)
DEFINES+=$(DEFINES_LIVE555) -D__STDC_FORMAT_MACROS

-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
