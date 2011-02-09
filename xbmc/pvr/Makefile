INCLUDES=-I. -I.. -I../../ -I../linux -I../cores -I../../guilib -I../posix -I../utils -I../epg

SRCS=PVRManager.cpp \
	PVRDatabase.cpp \
	GUIViewStatePVR.cpp

LIB=pvr.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
