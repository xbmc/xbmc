INCLUDES=-I.
CFLAGS=-fPIC
SRCS=client.cpp \
		 PVRClient-vdr.cpp \
		 VTPSession.cpp
OBJS=$(SRCS:.cpp=.o)
LIB=XBMC_VDR.pvr

-include $(OBJS:.o=.P)

$(LIB): $(OBJS)
ifeq ($(ARCH), osx)
	@export MACOSX_DEPLOYMENT_TARGET=10.4
	$(CC) -bundle -shared -flat_namespace -undefined suppress -o $@ $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@

else
	g++ $(CFLAGS) -shared -g -o $(LIB) $(OBJS)

endif

.DEFAULT_GOAL:= $(LIB)
