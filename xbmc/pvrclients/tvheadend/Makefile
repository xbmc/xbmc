#
# Makefile for the XBMC HTS Tvheadend PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

INCLUDES = -I. -I../../../xbmc/cores/dvdplayer/DVDDemuxers -I../../../xbmc/addons/include -I../../../xbmc/lib
ifneq (@USE_EXTERNAL_FFMPEG@,1)
  INCLUDES+=-I../../cores/dvdplayer/Codecs/ffmpeg
endif
ifeq ($(findstring Darwin,$(shell uname -a)), Darwin)
INCLUDES +=  -I/opt/local/include
endif

DEFINES += -D_LINUX -fPIC -DUSE_DEMUX

SRCS=client.cpp \
	HTSPSession.cpp \
	HTSPData.cpp \
	HTSPDemux.cpp \
	thread.cpp \
	tools.cpp

LIBS=../../../xbmc/lib/libhts/libhts.a -ldl
LIBDIR=../../../addons/pvr.hts
LIB=../../../addons/pvr.hts/XBMC_Tvheadend.pvr

ifeq ($(findstring Darwin,$(shell uname -a)), Darwin)
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses -dynamiclib -single_module -undefined dynamic_lookup
else
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses
endif

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

$(LIB):
	${MAKE} -C ../../../xbmc/lib/libhts
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -g $(OBJS) $(LIBS) $(LIBDIRS) $(SILIB) -o $(LIB)
