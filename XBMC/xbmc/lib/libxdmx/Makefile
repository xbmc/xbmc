# Makefile for libxdmx 
#   The XBMC Demux Library
# Copyright (C) 2009 Team XBMC
# 

STATIC_LIB := libxdmx.a
SHARED_LIB := libxdmx.so
SRCS    := src/accum.cpp src/bitstream.cpp src/pesparse.cpp src/tsdemux.cpp src/xdmx.cpp
INCLUDE := -Iinclude/ -Isrc/
OBJS    := ${SRCS:.cpp=.o} 
DEPS    := ${SRCS:.cpp=.d} 
XDEPS   := $(wildcard ${DEPS}) 
CXX     := g++
CXXFLAGS:= -O2 -Wall -Werror -ggdb ${INCLUDE} -fPIC -fno-common
MAKEDEP := ${CXX} -M $(CXXFLAGS) -o $*.d $<
LDFLAGS := 
LIBS    := 

.PHONY: all clean distclean 
all:: ${STATIC_LIB} #${SHARED_LIB}

ifneq (${XDEPS},) 
include ${XDEPS}
endif 

${STATIC_LIB}: ${OBJS} 
	${AR} rs $@ $^

${SHARED_LIB}: ${OBJS} 
#	${CXX} ${LDFLAGS} -shared -Wl,-soname,$@ -o $@ $^ ${LIBS} 
	${CXX} ${LDFLAGS} -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-install_name,/usr/local/lib/$@ -o $@ $^ ${LIBS} 

${OBJS}: %.o: %.cpp %.d 
	${CXX} ${CXXFLAGS} -o $@ -c $< 

${DEPS}: %.d: %.cpp Makefile 
	${MAKEDEP} $< > $@ 

clean:: 
	-rm -f *~ .d ${OBJS} ${DEPS} ${STATIC_LIB} ${SHARED_LIB} 

distclean:: clean

