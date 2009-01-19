INCLUDES=-I. -I../ -I../../ -I../linux -I../../guilib -I../utils -Idvdplayer
ifneq (1,1)
  INCLUDES+=-Iffmpeg
endif

SRCS=DummyVideoPlayer.cpp \
     PlayerCoreFactory.cpp \
     ssrc.cpp \
     dlgcache.cpp \

LIB=cores.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
