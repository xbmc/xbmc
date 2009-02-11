INCLUDES=-I. -I../../linux -I../../
CFLAGS=-fPIC
SRCS=Main.cpp

LIB=../../../pvrclients/VDR.dll

include ../../../Makefile.include
-include $(OBJS:.o=.P)

$(LIB): $(OBJS)
ifeq ($(ARCH), osx)
	@export MACOSX_DEPLOYMENT_TARGET=10.4
	$(CC) -bundle -shared -flat_namespace -undefined suppress -o $@ $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@

else
	$(CXX) $(CFLAGS) -shared -g -o $(LIB) $(OBJS)

endif

.DEFAULT_GOAL:= $(LIB)
