#****************************************************************************
#
# Makefil for TinyXml test.
# Lee Thomason
# www.grinninglizard.com
#
# This is a GNU make (gmake) makefile
#****************************************************************************

# DEBUG can be set to YES to include debugging info, or NO otherwise
DEBUG          := YES

# PROFILE can be set to YES to include profiling info, or NO otherwise
PROFILE        := NO

#****************************************************************************

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CFLAGS     := -Wall -Wno-unknown-pragmas -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O2

LIBS		 :=

DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS} 
RELEASE_CXXFLAGS := ${RELEASE_CFLAGS}

DEBUG_LDFLAGS    := -g
RELEASE_LDFLAGS  :=

ifeq (YES, ${DEBUG})
   CFLAGS       := ${DEBUG_CFLAGS}
   CXXFLAGS     := ${DEBUG_CXXFLAGS}
   LDFLAGS      := ${DEBUG_LDFLAGS}
else
   CFLAGS       := ${RELEASE_CFLAGS}
   CXXFLAGS     := ${RELEASE_CXXFLAGS}
   LDFLAGS      := ${RELEASE_LDFLAGS}
endif

ifeq (YES, ${PROFILE})
   CFLAGS   := ${CFLAGS} -pg
   CXXFLAGS := ${CXXFLAGS} -pg
   LDFLAGS  := ${LDFLAGS} -pg
endif

#****************************************************************************
# Preprocessor directives
#****************************************************************************

ifeq (YES, ${PROFILE})
  DEFS :=
else
  DEFS :=
endif

#****************************************************************************
# Include paths
#****************************************************************************

#INCS := -I/usr/include/g++-2 -I/usr/local/include
INCS :=


#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS   := ${CFLAGS}   ${DEFS}
CXXFLAGS := ${CXXFLAGS} ${DEFS}

#****************************************************************************
# Targets of the build
#****************************************************************************

OUTPUT := xmltest

all: ${OUTPUT}


#****************************************************************************
# Source files
#****************************************************************************

SRCS := tinyxml.cpp tinyxmlparser.cpp xmltest.cpp tinyxmlerror.cpp

# Add on the sources for libraries
SRCS := ${SRCS}

OBJS := $(addsuffix .o,$(basename ${SRCS}))

#****************************************************************************
# Output
#****************************************************************************

${OUTPUT}: ${OBJS}
	${LD} -o $@ ${LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}

#****************************************************************************
# common rules
#****************************************************************************

# Rules for compiling source files to object files
%.o : %.cpp
	${CXX} -c ${CXXFLAGS} ${INCS} $< -o $@

%.o : %.c
	${CC} -c ${CFLAGS} ${INCS} $< -o $@

clean:
	-rm -f core ${OBJS} ${OUTPUT}

depend:
	makedepend ${INCS} ${SRCS}
# DO NOT DELETE

tinyxml.o: tinyxml.h /usr/include/stdio.h /usr/include/features.h
tinyxml.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
tinyxml.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
tinyxml.o: /usr/include/bits/sched.h /usr/include/libio.h
tinyxml.o: /usr/include/_G_config.h /usr/include/wchar.h
tinyxml.o: /usr/include/bits/wchar.h /usr/include/gconv.h
tinyxml.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h
tinyxmlparser.o: tinyxml.h /usr/include/stdio.h /usr/include/features.h
tinyxmlparser.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
tinyxmlparser.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
tinyxmlparser.o: /usr/include/bits/sched.h /usr/include/libio.h
tinyxmlparser.o: /usr/include/_G_config.h /usr/include/wchar.h
tinyxmlparser.o: /usr/include/bits/wchar.h /usr/include/gconv.h
tinyxmlparser.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h
tinyxmlparser.o: /usr/include/ctype.h /usr/include/endian.h
tinyxmlparser.o: /usr/include/bits/endian.h
xmltest.o: tinyxml.h /usr/include/stdio.h /usr/include/features.h
xmltest.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
xmltest.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
xmltest.o: /usr/include/bits/sched.h /usr/include/libio.h
xmltest.o: /usr/include/_G_config.h /usr/include/wchar.h
xmltest.o: /usr/include/bits/wchar.h /usr/include/gconv.h
xmltest.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h
tinyxmlerror.o: tinyxml.h /usr/include/stdio.h /usr/include/features.h
tinyxmlerror.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
tinyxmlerror.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
tinyxmlerror.o: /usr/include/bits/sched.h /usr/include/libio.h
tinyxmlerror.o: /usr/include/_G_config.h /usr/include/wchar.h
tinyxmlerror.o: /usr/include/bits/wchar.h /usr/include/gconv.h
tinyxmlerror.o: /usr/include/bits/stdio_lim.h /usr/include/assert.h
