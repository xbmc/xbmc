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
INCLUDES = -I. -I../../linux -I../../ -I../../../xbmc/addons/include -I../../../addons/library.xbmc.pvr -I../../../addons/library.xbmc.addon
DEFINES += -D_LINUX -fPIC
LIBDIR   = ../../../addons/pvr.team-mediaportal.tvserver
LIB      = ../../../addons/pvr.team-mediaportal.tvserver/XBMC_MPTV.pvr

CC       ?= gcc
CFLAGS   ?= -g -O2 -Wall

CXX      ?= g++
ifeq ($(findstring Darwin,$(shell uname -a)), Darwin)
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses -Wl,-no_compact_linkedit -dynamiclib -single_module -undefined dynamic_lookup -no_compact_linkedit
DEFINES += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common
else
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses
endif

-include Make.config

OBJS = channels.o client.o epg.o pvrclient-mediaportal.o recordings.o timers.o utils.o Socket.o

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
	$(CXX) $(CXXFLAGS) -shared -g $(OBJS) $(LIBS) $(LIBDIRS) $(SILIB) -o $(LIB)

# Install the files:

install: install-lib

# PVR library:

install-lib: $(LIB)
	@mkdir -p $(DESTDIR)$(ADDONDIR)
	@cp --remove-destination -r $(LIBDIR) $(DESTDIR)$(ADDONDIR)

clean:
	-rm -f $(OBJS) $(DEPFILE) $(LIB) *~
CLEAN: clean
