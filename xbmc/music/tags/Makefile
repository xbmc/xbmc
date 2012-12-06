SRCS=\
     MusicInfoTag.cpp \
     MusicInfoTagLoaderASAP.cpp \
     MusicInfoTagLoaderCDDA.cpp \
     MusicInfoTagLoaderDatabase.cpp \
     MusicInfoTagLoaderFactory.cpp \
     MusicInfoTagLoaderMidi.cpp \
     MusicInfoTagLoaderNSF.cpp \
     MusicInfoTagLoaderShn.cpp \
     MusicInfoTagLoaderSPC.cpp \
     MusicInfoTagLoaderWav.cpp \
     MusicInfoTagLoaderYM.cpp \
     TagLoaderTagLib.cpp \
     TagLibVFSStream.cpp \

LIB=musictags.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
