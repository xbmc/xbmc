INCLUDES=-I. -I.. -I../../ -I../linux -I../cores -I../../guilib -I../posix -I../utils

SRCS=PVRChannel.cpp \
	PVRChannelGroup.cpp \
	PVRChannelGroups.cpp \
	PVRChannels.cpp \
	PVREpg.cpp \
	PVREpgInfoTag.cpp \
	PVREpgs.cpp \
	PVREpgSearchFilter.cpp \
	PVRManager.cpp \
	PVRRecordings.cpp \
	PVRTimerInfoTag.cpp \
	PVRTimers.cpp \
	TVDatabase.cpp

LIB=pvr.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
