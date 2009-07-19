#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under difference licenses.  The codec libraries
#  are distributed under Xiph.Org's BSD-like license (see the file
#  COPYING.Xiph in this distribution).  All other programs, libraries, and
#  plugins are distributed under the GPL (see COPYING.GPL).  The documentation
#  is distributed under the Gnu FDL (see COPYING.FDL).  Each file in the
#  FLAC distribution contains at the top the terms under which it may be
#  distributed.
#
#  Since this particular file is relevant to all components of FLAC,
#  it may be distributed under the Xiph.Org license, which is the least
#  restrictive of those mentioned above.  See the file COPYING.Xiph in this
#  distribution.

#
# GNU makefile fragment for building an executable
#

include $(topdir)/build/config.mk

ifeq ($(DARWIN_BUILD),yes)
CC          = cc
CCC         = c++
else
CC          = gcc
CCC         = g++
endif
NASM        = nasm
LINK        = $(CC) $(LINKAGE)
OBJPATH     = $(topdir)/obj
BINPATH     = $(OBJPATH)/$(BUILD)/bin
LIBPATH     = $(OBJPATH)/$(BUILD)/lib
DEBUG_BINPATH   = $(OBJPATH)/debug/bin
DEBUG_LIBPATH   = $(OBJPATH)/debug/lib
RELEASE_BINPATH = $(OBJPATH)/release/bin
RELEASE_LIBPATH = $(OBJPATH)/release/lib
PROGRAM         = $(BINPATH)/$(PROGRAM_NAME)
DEBUG_PROGRAM   = $(DEBUG_BINPATH)/$(PROGRAM_NAME)
RELEASE_PROGRAM = $(RELEASE_BINPATH)/$(PROGRAM_NAME)

debug   : CFLAGS = -g -O0 -DDEBUG $(CONFIG_CFLAGS) $(DEBUG_CFLAGS) -W -Wall -Wmissing-prototypes -Wstrict-prototypes -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)
valgrind: CFLAGS = -g -O0 -DDEBUG $(CONFIG_CFLAGS) $(DEBUG_CFLAGS) -DFLAC__VALGRIND_TESTING -W -Wall -Wmissing-prototypes -Wstrict-prototypes -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -DNDEBUG $(CONFIG_CFLAGS) $(RELEASE_CFLAGS) -W -Wall -Wmissing-prototypes -Wstrict-prototypes -Winline -DFLaC__INLINE=__inline__ -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

DEBUG_OBJS = $(SRCS_C:%.c=%.debug.o) $(SRCS_CC:%.cc=%.debug.o) $(SRCS_CPP:%.cpp=%.debug.o) $(SRCS_NASM:%.nasm=%.debug.o)
RELEASE_OBJS = $(SRCS_C:%.c=%.release.o) $(SRCS_CC:%.cc=%.release.o) $(SRCS_CPP:%.cpp=%.release.o) $(SRCS_NASM:%.nasm=%.release.o)

debug   : $(DEBUG_PROGRAM)
valgrind: $(DEBUG_PROGRAM)
release : $(RELEASE_PROGRAM)

# by default on OS X we link with static libs as much as possible

$(DEBUG_PROGRAM) : $(DEBUG_OBJS)
ifeq ($(DARWIN_BUILD),yes)
	$(LINK) -o $@ $(DEBUG_OBJS) $(EXPLICIT_LIBS)
else
	$(LINK) -o $@ $(DEBUG_OBJS) $(LFLAGS) $(LIBS)
endif

$(RELEASE_PROGRAM) : $(RELEASE_OBJS)
ifeq ($(DARWIN_BUILD),yes)
	$(LINK) -o $@ $(RELEASE_OBJS) $(EXPLICIT_LIBS)
else
	$(LINK) -o $@ $(RELEASE_OBJS) $(LFLAGS) $(LIBS)
endif

%.debug.o %.release.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.debug.o %.release.o : %.cc
	$(CCC) $(CFLAGS) -c $< -o $@
%.debug.o %.release.o : %.cpp
	$(CCC) $(CFLAGS) -c $< -o $@
%.debug.i %.release.i : %.c
	$(CC) $(CFLAGS) -E $< -o $@
%.debug.i %.release.i : %.cc
	$(CCC) $(CFLAGS) -E $< -o $@
%.debug.i %.release.i : %.cpp
	$(CCC) $(CFLAGS) -E $< -o $@

%.debug.o : %.nasm
	$(NASM) -f elf -d OBJ_FORMAT_elf -i ia32/ -g $< -o $@
%.release.o : %.nasm
	$(NASM) -f elf -d OBJ_FORMAT_elf -i ia32/ $< -o $@

.PHONY : clean
clean :
	-rm -f $(DEBUG_OBJS) $(RELEASE_OBJS) $(OBJPATH)/*/bin/$(PROGRAM_NAME)

.PHONY : depend
depend:
	makedepend -fMakefile.lite -- $(CFLAGS) $(INCLUDES) -- *.c *.cc *.cpp
