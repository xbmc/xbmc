SRCS=\
     MusicInfoTag.cpp \
     MusicInfoTagLoaderCDDA.cpp \
     MusicInfoTagLoaderDatabase.cpp \
     MusicInfoTagLoaderFactory.cpp \
     MusicInfoTagLoaderShn.cpp \
     TagLoaderTagLib.cpp \
     TagLibVFSStream.cpp \
     ReplayGain.cpp \

LIB=musictags.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
