#
# Makefile for the XBMC Video Disk Recorder PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

.DELETE_ON_ERROR:

CC       ?= gcc
CFLAGS   ?= -g -O2 -Wall

CXX      ?= g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses

ARCH     = x86_64-linux
DESTDIR ?=
PREFIX  ?= /usr/local
ADDONDIR = $(PREFIX)/share/xbmc/addons
LIBS     = ../../addons/include/xbmc_addon_lib++.a ../../addons/include/xbmc_pvr_lib++.a
INCLUDES = -I. -I../../linux -I../../ -I ../../../guilib
DEFINES += -D_LINUX -fPIC
LIBDIR   = ../../../addons/pvr/VDR
LIB      = ../../../addons/pvr/VDR/XBMC_VDR.pvr

-include Make.config

OBJS = client.o pvrclient-vdr.o vtptransceiver.o timers.o channels.o recordings.o epg.o

all: pvrVDR

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

pvrVDR: $(OBJS) $(SILIB)
	$(CXX) $(CXXFLAGS) -shared -g $(OBJS) $(LIBS) $(LIBDIRS) $(SILIB) -o $(LIB)

# Install the files:

install: install-lib

# PVR library:

install-lib: pvrVDR
	@mkdir -p $(DESTDIR)$(ADDONDIR)
	@cp --remove-destination -r $(LIBDIR) $(DESTDIR)$(ADDONDIR)

clean:
	-rm -f $(OBJS) $(DEPFILE) $(LIB) *~
CLEAN: clean
