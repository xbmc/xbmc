INCLUDES=-I. -I.. -I../../../xbmc/linux -I../../../xbmc -I../../../guilib -I../../../visualisations -I/opt/local/include
SRCS=Main.cpp itunes_vis.c qview.m common_osx.m
CFLAGS=-Wall -g -fPIC
LIB=../../../visualisations/iTunes.mvis

include ../../../Makefile.include

-include $(OBJS:.o=.P)

$(LIB): $(OBJS)
ifeq ($(ARCH), osx)
	@export MACOSX_DEPLOYMENT_TARGET=10.4
	$(CC) -framework Carbon -bundle -shared -flat_namespace -undefined suppress -o $@ $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@
endif

.DEFAULT_GOAL:= $(LIB)
