#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

.DELETE_ON_ERROR:

DESTDIR ?=
PREFIX  ?= /usr/local
ADDONDIR = $(PREFIX)/share/xbmc/addons
LIBS     = -ldl
INCLUDES = -I. -I../../../xbmc/cores/dvdplayer/DVDDemuxers -I../../../xbmc/addons/include -I../../../xbmc/cores/dvdplayer/Codecs/ffmpeg
ifeq ($(findstring Darwin,$(shell uname -a)), Darwin)
INCLUDES +=  -I/opt/local/include -I../../
DEFINES += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common
endif
DEFINES += -D_LINUX -fPIC -DUSE_DEMUX
LIBDIR   = ../../../addons/pvr.vdr.vnsi
LIB      = ../../../addons/pvr.vdr.vnsi/XBMC_VDR_vnsi.pvr

CC       ?= gcc
CFLAGS   ?= -g -O2 -Wall

CXX      ?= g++
ifeq ($(findstring Darwin,$(shell uname -a)), Darwin)
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses -Wl,-no_compact_linkedit -dynamiclib -single_module -undefined dynamic_lookup
else
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses
endif

-include Make.config

OBJS = client.o VNSIChannelScan.o VNSIData.o VNSIDemux.o VNSIRecording.o VNSISession.o recordings.o requestpacket.o responsepacket.o thread.o tools.o

all: $(LIB)

# Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

# The main library:

$(LIB): $(OBJS) $(SILIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -g $(OBJS) $(LIBS) $(LIBDIRS) $(SILIB) -o $(LIB)

# Install the files:

install: install-lib

# PVR library:

install-lib: $(LIB)
	@mkdir -p $(DESTDIR)$(ADDONDIR)
	@cp --remove-destination -r $(LIBDIR) $(DESTDIR)$(ADDONDIR)

clean:
	-rm -f $(OBJS) $(DEPFILE) $(LIB) *~
CLEAN: clean
