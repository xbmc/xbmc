INCLUDES=-I. -I../ -I../linux -I../../guilib -I../utils -I../cores/ffmpeg -I../../include

#SRCS=AddonHelpers_Addon.cpp \
#     AddonHelpers_GUI.cpp \
#     AddonHelpers_PVR.cpp \
#     AddonHelpers_Vis.cpp \
SRCS= AddonHelpers_local.cpp\

LIB=AddonHelpers.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

