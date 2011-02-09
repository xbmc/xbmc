INCLUDES=-I. -I.. -I../../ -I../linux -I../cores -I../../guilib -I../posix -I../utils -I../epg

SRCS=PVRChannel.cpp \
	PVRChannelGroup.cpp \
	PVRChannelGroupInternal.cpp \
	PVRChannelGroups.cpp \
	PVRChannelGroupsContainer.cpp \
	PVRManager.cpp \
	PVRTimerInfoTag.cpp \
	PVRTimers.cpp \
	PVRDatabase.cpp \
	GUIViewStatePVR.cpp

LIB=pvr.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
