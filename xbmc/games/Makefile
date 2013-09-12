SRCS=GameClient.cpp \
     GameFileAutoLauncher.cpp \
     GameFileLoader.cpp \
     GameManager.cpp \
     GameSettings.cpp \
     SerialState.cpp

LIB=games.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
