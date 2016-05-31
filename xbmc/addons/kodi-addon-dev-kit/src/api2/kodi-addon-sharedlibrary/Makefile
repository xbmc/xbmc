# WARNING: This Makefile is only be used on Kodi's own build. If you use it
# independent is cmake needed for creation!
#

### The archiver options:

AR       ?= ar
ARFLAGS  ?= ru

INCLUDES=-I. \
         -I../../../../../ \
         -I../../../../../cores/VideoPlayer/DVDDemuxers \
         -I../../../include/

DEFINES+=
CXXFLAGS=-fPIC -std=c++11
LIBNAME=libkodi.addon.sharedlibrary.api2.a

### The object files (add further files here):

OBJS=Interface.o \
     AddonLib.o \
     ErrorCodeNames.o

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

all: $(LIBNAME)

### Targets:

$(LIBNAME): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

CLEAN_FILES = \
	$(LIBNAME) \

include ../../../../../../Makefile.include
