#
# Makefile for the XBMC MediaPortal PVR AddOn
#
# See the README for copyright information and
# how to reach the author.
#

.DELETE_ON_ERROR:

DESTDIR ?=
PREFIX  ?= /usr/local
ADDONDIR = $(PREFIX)/share/xbmc/addons
LIBS     = -ldl
INCLUDES = -I. -I../../linux -I../../ -I../../../xbmc/addons/include -I../../../addons/org.xbmc.pvr.library -I../../../addons/org.xbmc.addon.library
DEFINES += -D_LINUX -fPIC
LIBDIR   = ../../../addons/de.team-mediaportal.tvserver.pvr
LIB      = ../../../addons/de.team-mediaportal.tvserver.pvr/XBMC_MPTV.pvr

CC       ?= gcc
CFLAGS   ?= -g -O2 -Wall

CXX      ?= g++
ifeq ($(findstring Darwin,$(shell uname -a)), Darwin)
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses -dynamiclib -single_module -undefined dynamic_lookup
DEFINES += -D__APPLE__
else
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses
endif

-include Make.config

OBJS = channels.o client.o epg.o pvrclient-mediaportal.o recordings.o timers.o utils.o Socket.o

all: pvrMPTV

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

pvrMPTV: $(OBJS) $(SILIB)
	$(CXX) $(CXXFLAGS) -shared -g $(OBJS) $(LIBS) $(LIBDIRS) $(SILIB) -o $(LIB)

# Install the files:

install: install-lib

# PVR library:

install-lib: pvrMPTV
	@mkdir -p $(DESTDIR)$(ADDONDIR)
	@cp --remove-destination -r $(LIBDIR) $(DESTDIR)$(ADDONDIR)

clean:
	-rm -f $(OBJS) $(DEPFILE) $(LIB) *~
CLEAN: clean
