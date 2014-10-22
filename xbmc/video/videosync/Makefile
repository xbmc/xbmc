SRCS=VideoSyncGLX.cpp \
     VideoSyncCocoa.cpp \
     VideoSyncDRM.cpp \
     VideoSyncPi.cpp \

LIB=videosync.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))