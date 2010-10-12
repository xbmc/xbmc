INCLUDES +=  -I. -I../../ -I../../../linux -I../../../ -I../../../../xbmc/addons/include -I../../../../guilib
DEFINES += -D_LINUX -fPIC

OBJS = GetChannelListResult.o \
       GetNumChannelsResult.o \
       GetProgramGuideParameters.o \
       MythXmlCommand.o \
       MythXmlCommandParameters.o 

LIB=libmythxml.a

# all is the default rule
all: $(LIB)

$(LIB): $(OBJS)
	rm -f $@
	ar cq $@ $(OBJS)

clean:
	-rm -f $(OBJS) $(LIB) *~
CLEAN: clean

include ../../../../Makefile.include

