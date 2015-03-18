SRCS=AddonEvent.cpp \
     AddonManagementEvent.cpp \
     BaseEvent.cpp \
     EventLog.cpp \
     MediaLibraryEvent.cpp \
     windows/GUIViewStateEventLog.cpp \
     windows/GUIWindowEventLog.cpp \

LIB=events.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

