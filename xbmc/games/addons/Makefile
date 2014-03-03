SRCS=GameClient.cpp \
     GameClientInput.cpp \
     GameClientKeyboard.cpp \
     GameClientMouse.cpp \
     GameClientProperties.cpp \
     GameClientTiming.cpp \
     GameClientTranslator.cpp \

LIB=gameaddons.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
