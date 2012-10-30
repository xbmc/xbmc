SRCS=AddonsOperations.cpp \
     ApplicationOperations.cpp \
     AudioLibrary.cpp \
     FileItemHandler.cpp \
     FileOperations.cpp \
     GUIOperations.cpp \
     InputOperations.cpp \
     JSONRPC.cpp \
     JSONServiceDescription.cpp \
     PlayerOperations.cpp \
     PlaylistOperations.cpp \
     PVROperations.cpp \
     SystemOperations.cpp \
     VideoLibrary.cpp \
     XBMCOperations.cpp \

LIB=json-rpc.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
