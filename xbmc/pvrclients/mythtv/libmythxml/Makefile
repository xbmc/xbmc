INCLUDES +=  -I. -I../../ -I../../../linux -I../../../ -I../../../../xbmc/addons/include -I../../../../guilib
DEFINES += -D_LINUX -fPIC

OBJS = GetProgramGuideResult.o \
       GetChannelListResult.o \
       GetNumChannelsResult.o \
       GetProgramGuideParameters.o \
       MythXmlCommand.o \
       MythXmlCommandParameters.o 

LIB = libmythxml.a

# all is the default rule
all: $(LIB)

include ../../../../Makefile.include
