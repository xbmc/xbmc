ARCH=@ARCH@
INCLUDES=-I. -I../../../xbmc/addons/include -I../../../xbmc
DEFINES+=
CXXFLAGS=-fPIC
LIBNAME=libXBMC_addon
OBJS=$(LIBNAME).o

LIB_SHARED=../../../addons/library.xbmc.addon/$(LIBNAME)-$(ARCH).so

all: $(LIB_SHARED)

$(LIB_SHARED): $(OBJS)
ifeq ($(findstring osx,$(ARCH)), osx)
	@export MACOSX_DEPLOYMENT_TARGET=10.4
	$(CXX) -bundle -shared -flat_namespace -undefined suppress -o $(LIB_SHARED) $(OBJS)
	../../../tools/Mach5/wrapper.rb $@;mv output.so $@
else
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -g -o $(LIB_SHARED) $(OBJS)
endif

CLEAN_FILES = \
	$(LIBNAME).so \

DISTCLEAN_FILES= \
	Makefile \

include ../../../Makefile.include
