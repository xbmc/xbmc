SRCS=GUIViewStatePVR.cpp \
     GUIWindowPVRBase.cpp \
     GUIWindowPVRChannels.cpp \
     GUIWindowPVRGuide.cpp \
     GUIWindowPVRRecordings.cpp \
     GUIWindowPVRSearch.cpp \
     GUIWindowPVRTimersBase.cpp \
     GUIWindowPVRTimers.cpp \
     GUIWindowPVRTimerRules.cpp

LIB=pvrwindows.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
