ARCH=x86_64-linux
INCLUDES=-I. -I../../linux -I../../ -I ../../../guilib
DEFINES+=
CXXFLAGS=-fPIC
OBJS=client.o pvrclient-vdr.o vtptransceiver.o

SLIB=../../../addons/pvr/VDR/vdr.pvr

$(SLIB): $(OBJS)
ifeq ($(findstring osx,$(ARCH)), osx)
	@export MACOSX_DEPLOYMENT_TARGET=10.4
	$(CXX) -bundle -shared -flat_namespace -undefined suppress -o $(SLIB) $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@
else
	$(CXX) $(CFLAGS) -shared -g -o $(SLIB) $(OBJS)
endif

include ../../../Makefile.include
